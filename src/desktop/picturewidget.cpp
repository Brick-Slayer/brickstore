/* Copyright (C) 2004-2022 Robert Griebl. All rights reserved.
**
** This file is part of BrickStore.
**
** This file may be distributed and/or modified under the terms of the GNU
** General Public License version 2 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://fsf.org/licensing/licenses/gpl.html for GPL licensing information.
*/
#include <QLabel>
#include <QPalette>
#include <QMenu>
#include <QLayout>
#include <QVector>
#include <QApplication>
#include <QAction>
#include <QDesktopServices>
#include <QHelpEvent>
#include <QToolButton>
#include <QStyle>
#include <QClipboard>
#include <QFileDialog>
#include <QStringBuilder>
#include <QQmlApplicationEngine>

#include "bricklink/color.h"
#include "bricklink/core.h"
#include "bricklink/delegate.h"
#include "bricklink/item.h"
#include "bricklink/picture.h"
#include "common/application.h"
#include "common/config.h"
#include "common/eventfilter.h"
#include "common/systeminfo.h"
#include "ldraw/library.h"
#include "ldraw/part.h"
#include "ldraw/renderwidget.h"
#include "qcoro/qcorosignal.h"
#include "picturewidget.h"
#include "rendersettingsdialog.h"


static bool isGPUBlackListed()
{
    // these GPUs will crash QtQuick3D on Windows
    static const QVector<QByteArray> gpuBlacklist = {
        "Microsoft Basic Render Driver",
        "Intel(R) HD Graphics",
        "Intel(R) HD Graphics 3000",
        "Intel(R) Q45/Q43 Express Chipset (Microsoft Corporation - WDDM 1.1)",
        "NVIDIA GeForce 210",
        "NVIDIA nForce 980a/780a SLI",
        "NVIDIA GeForce GT 525M",
        "NVIDIA GeForce 8400 GS",
        "NVIDIA NVS 5100M",
        "NVIDIA Quadro 1000M",
        "AMD Radeon HD 8240",
    };
    const auto gpu = SystemInfo::inst()->asMap().value(u"hw.gpu"_qs).toString();
    bool blacklisted = gpuBlacklist.contains(gpu.toLatin1());
    qWarning() << "Checking if GPU" << gpu << "is blacklisted:" << (blacklisted ? "yes" : "no");
    return blacklisted;
}


PictureWidget::PictureWidget(QWidget *parent)
    : QFrame(parent)
{
    setBackgroundRole(QPalette::Base);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setAutoFillBackground(true);

    w_text = new QLabel();
    w_text->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    w_text->setWordWrap(true);
    w_text->setTextInteractionFlags(Qt::TextSelectableByMouse);
    w_text->setContextMenuPolicy(Qt::DefaultContextMenu);

    new EventFilter(w_text, { QEvent::ContextMenu }, [this](QObject *, QEvent *) {
        w_text->setContextMenuPolicy(w_text->hasSelectedText() ? Qt::DefaultContextMenu
                                                               : Qt::NoContextMenu);
        return EventFilter::ContinueEventProcessing;
    });
    new EventFilter(w_text, { QEvent::Resize }, [this](QObject *, QEvent *) {
        // workaround for layouts breaking, if a rich-text label with word-wrap has
        // more than one line
        QMetaObject::invokeMethod(w_text, [this]() {
            w_text->setMinimumHeight(0);
            int h = w_text->heightForWidth(w_text->width());
            if (h > 0)
                w_text->setMinimumHeight(h);
        }, Qt::QueuedConnection);

        return EventFilter::ContinueEventProcessing;
    });

    w_image = new QLabel();
    w_image->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    w_image->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    w_image->setMinimumSize(BrickLink::core()->standardPictureSize());
    w_image->setAutoFillBackground(true);

    w_ldraw = new LDraw::RenderWidget(Application::inst()->qmlEngine(), this);
    w_ldraw->hide();

    auto layout = new QVBoxLayout(this);
    layout->addWidget(w_text);
    layout->addWidget(w_image, 10);
    layout->addWidget(w_ldraw, 10);
    layout->setContentsMargins(2, 6, 2, 2);

    w_2d = new QToolButton();
    w_2d->setText(u"2D"_qs);
    w_2d->setAutoRaise(true);
    w_2d->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    connect(w_2d, &QToolButton::clicked, this, [this]() {
        m_prefer3D = false;
        redraw();
    });

    w_3d = new QToolButton();
    w_3d->setText(u"3D"_qs);
    w_3d->setAutoRaise(true);
    w_3d->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    connect(w_3d, &QToolButton::clicked, this, [this]() {
        m_prefer3D = true;
        redraw();
    });

    auto font = w_2d->font();
    font.setBold(true);
    w_2d->setFont(font);
    w_3d->setFont(font);

    m_rescaleIcon = QIcon::fromTheme(u"zoom-fit-best"_qs);
    m_reloadIcon = QIcon::fromTheme(u"view-refresh"_qs);

    w_reloadRescale = new QToolButton();
    w_reloadRescale->setAutoRaise(true);
    w_reloadRescale->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    w_reloadRescale->setIcon(m_rescaleIcon);
    connect(w_reloadRescale, &QToolButton::clicked,
            this, [this]() {
        if (m_is3D) {
            w_ldraw->resetCamera();
        } else if (m_pic) {
            m_pic->update(true);
            redraw();
        }
    });

    w_2d->setEnabled(false);
    w_3d->setEnabled(false);
    w_reloadRescale->setEnabled(false);

    auto buttons = new QHBoxLayout();
    buttons->setContentsMargins(0, 0, 0, 0);
    buttons->addWidget(w_2d, 10);
    buttons->addWidget(w_3d, 10);
    buttons->addWidget(w_reloadRescale, 10);
    layout->addLayout(buttons);

    m_blCatalog = new QAction(QIcon::fromTheme(u"bricklink-catalog"_qs), { }, this);
    connect(m_blCatalog, &QAction::triggered, this, [this]() {
        BrickLink::core()->openUrl(BrickLink::Url::CatalogInfo, m_item, m_color);
    });

    m_blPriceGuide = new QAction(QIcon::fromTheme(u"bricklink-priceguide"_qs), { }, this);
    connect(m_blPriceGuide, &QAction::triggered, this, [this]() {
        BrickLink::core()->openUrl(BrickLink::Url::PriceGuideInfo, m_item, m_color);
    });

    m_blLotsForSale = new QAction(QIcon::fromTheme(u"bricklink-lotsforsale"_qs), { }, this);
    connect(m_blLotsForSale, &QAction::triggered, this, [this]() {
        BrickLink::core()->openUrl(BrickLink::Url::LotsForSale, m_item, m_color);
    });
    m_renderSettings = new QAction({ }, this);
    connect(m_renderSettings, &QAction::triggered, this, []() {
        RenderSettingsDialog::inst()->show();
    });

    m_copyImage = new QAction(QIcon::fromTheme(u"edit-copy"_qs), { }, this);
    connect(m_copyImage, &QAction::triggered, this, [this]() -> QCoro::Task<> {
                QImage img;
                if (w_ldraw->isVisible()) {
                    if (w_ldraw->startGrab()) {
                        img = co_await qCoro(w_ldraw, &LDraw::RenderWidget::grabFinished);
                    }
                } else if (!m_image.isNull()) {
                    img = m_image;
                }
                auto clip = QGuiApplication::clipboard();
                clip->setImage(img);
            });

    m_saveImageAs = new QAction(QIcon::fromTheme(u"document-save"_qs), { }, this);
    connect(m_saveImageAs, &QAction::triggered, this, [this]() -> QCoro::Task<> {
                QImage img;
                if (w_ldraw->isVisible()) {
                    if (w_ldraw->startGrab()) {
                        img = co_await qCoro(w_ldraw, &LDraw::RenderWidget::grabFinished);
                    }
                } else if (!m_image.isNull()) {
                    img = m_image;
                }
                QStringList filters;
                filters << tr("PNG Image") % u" (*.png)";

                QString fn = QFileDialog::getSaveFileName(this, tr("Save image as"),
                Config::inst()->lastDirectory(),
                filters.join(u";;"));
                if (!fn.isEmpty()) {
                    Config::inst()->setLastDirectory(QFileInfo(fn).absolutePath());

                    if (fn.right(4) != u".png") {
                        fn += u".png"_qs;
                    }
                    img.save(fn, "PNG");
                }
            });

    connect(BrickLink::core(), &BrickLink::Core::pictureUpdated,
            this, [this](BrickLink::Picture *pic) {
        if (pic == m_pic) {
            if (pic->isValid())
                m_image = pic->image();
            redraw();
        }
    });

    connect(LDraw::library(), &LDraw::Library::libraryAboutToBeReset,
            this, [this]() {
        if (m_part) {
            m_part->release();
            m_part = nullptr;
            redraw();
        }
    });

    paletteChange();
    languageChange();
    redraw();
}

void PictureWidget::languageChange()
{
    m_renderSettings->setText(tr("3D render settings..."));
    m_copyImage->setText(tr("Copy image"));
    m_saveImageAs->setText(tr("Save image as..."));
    m_blCatalog->setText(tr("Show BrickLink Catalog Info..."));
    m_blPriceGuide->setText(tr("Show BrickLink Price Guide Info..."));
    m_blLotsForSale->setText(tr("Show Lots for Sale on BrickLink..."));
}

void PictureWidget::paletteChange()
{
    if (w_image) {
        auto pal = w_image->palette();
        pal.setColor(w_image->backgroundRole(), Qt::white);
        w_image->setPalette(pal);
    }
    if (w_ldraw) {
        auto pal = w_ldraw->palette();
        pal.setColor(w_ldraw->backgroundRole(), Qt::white);
        w_ldraw->setPalette(pal);
    }
}

void PictureWidget::resizeEvent(QResizeEvent *e)
{
    QFrame::resizeEvent(e);
    redraw();
}

PictureWidget::~PictureWidget()
{
    if (m_pic)
        m_pic->release();
    if (m_part)
        m_part->release();
}

void PictureWidget::setItemAndColor(const BrickLink::Item *item, const BrickLink::Color *color)
{
    if ((item == m_item) && (color == m_color))
        return;

    m_item = item;
    m_color = color;
    m_image = { };

    if (m_pic)
        m_pic->release();
    m_pic = item ? BrickLink::core()->picture(item, color, true) : nullptr;
    if (m_pic) {
        m_pic->addRef();
        if (m_pic->isValid())
            m_image = m_pic->image();
    }

    static bool badGPU = isGPUBlackListed();

    if (m_part)
        m_part->release();
    m_part = (item && !badGPU) ? LDraw::library()->partFromId(item->id()) : nullptr;
    if (m_part)
        m_part->addRef();

    m_blCatalog->setVisible(item);
    m_blPriceGuide->setVisible(item && color);
    m_blLotsForSale->setVisible(item && color);
    redraw();
}

bool PictureWidget::prefer3D() const
{
    return m_prefer3D;
}

void PictureWidget::setPrefer3D(bool b)
{
    if (m_prefer3D != b) {
        m_prefer3D = b;
        redraw();
    }
}

void PictureWidget::redraw()
{
    w_image->setPixmap({ });

    QString s = BrickLink::Core::itemHtmlDescription(m_item, m_color,
                                                     palette().color(QPalette::Highlight));
    w_text->setText(s);

    if (m_pic && (m_pic->updateStatus() == BrickLink::UpdateStatus::Updating)) {
        w_image->setText(u"<center><i>" % tr("Please wait... updating") % u"</i></center>");
    } else if (m_pic) {
        bool hasImage = !m_image.isNull();
        auto dpr = devicePixelRatioF();
        QPixmap p;
        QSize pSize;
        QSize displaySize = w_image->contentsRect().size();

        if (hasImage) {
            p = QPixmap::fromImage(m_image, Qt::NoFormatConversion);
            pSize = p.size();
        } else {
            pSize = BrickLink::core()->standardPictureSize();
        }
        QSize sz = pSize.scaled(displaySize, Qt::KeepAspectRatio).boundedTo(pSize * 2) * dpr;

        if (hasImage)
            p = p.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        else
            p = QPixmap::fromImage(BrickLink::core()->noImage(sz));

        p.setDevicePixelRatio(dpr);
        w_image->setPixmap(p);

    } else {
        w_image->setText({ });
    }

    m_is3D = m_part && m_prefer3D;

    if (m_is3D) {
        w_image->hide();
        w_ldraw->show();
        w_ldraw->setPartAndColor(m_part, m_color);
    } else {
        w_ldraw->setPartAndColor(nullptr, -1);
        w_ldraw->hide();
        w_ldraw->stopAnimation();
        w_image->show();
    }
    w_reloadRescale->setIcon(m_is3D ? m_rescaleIcon : m_reloadIcon);
    w_reloadRescale->setToolTip(m_is3D ? tr("Center view") : tr("Update"));
    m_renderSettings->setVisible(m_is3D);

    auto markText = [](const char *text, bool marked) {
        QString str = QString::fromLatin1(text);
        if (marked) {
            str.prepend(u'\x2308');
            str.append(u'\x230b');
        }
        return str;
    };

    w_2d->setText(markText("2D", !m_is3D && m_item));
    w_3d->setText(markText("3D", m_is3D));
    w_2d->setEnabled(m_is3D);
    w_3d->setEnabled(!m_is3D && m_part);
    w_reloadRescale->setEnabled(m_item);
}

void PictureWidget::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        languageChange();
    else if (e->type() == QEvent::PaletteChange)
        paletteChange();
    QFrame::changeEvent(e);
}

bool PictureWidget::event(QEvent *e)
{
    if (m_item && e && e->type() == QEvent::ToolTip) {
        return BrickLink::ToolTip::inst()->show(m_item, m_color,
                                                static_cast<QHelpEvent *>(e)->globalPos(), this);
    }
    return QFrame::event(e);
}

void PictureWidget::contextMenuEvent(QContextMenuEvent *e)
{
    if (m_item) {
        if (!m_contextMenu) {
            m_contextMenu = new QMenu(this);
            m_contextMenu->addAction(m_blCatalog);
            m_contextMenu->addAction(m_blPriceGuide);
            m_contextMenu->addAction(m_blLotsForSale);
            m_contextMenu->addSeparator();
            m_contextMenu->addAction(m_renderSettings);
            m_contextMenu->addSeparator();
            m_contextMenu->addAction(m_copyImage);
            m_contextMenu->addAction(m_saveImageAs);
        }
        m_contextMenu->popup(e->globalPos());
    }
    e->accept();
}

#include "moc_picturewidget.cpp"
