/* Copyright (C) 2004-2020 Robert Griebl. All rights reserved.
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
#include <QStyle>
#include <QStylePainter>
#include <QPushButton>
#include <QGroupBox>
#include <QTimer>
#include <QLayout>
#include <QEvent>
#include <QLabel>
#include <QFileInfo>
#include <QTextLayout>
#include <QtMath>
#include <QStaticText>
#include <QMenu>

#include "welcomewidget.h"
#include "config.h"
#include "humanreadabletimedelta.h"
#include "framework.h"
#include "version.h"
#include "ldraw.h"
#include "ldraw/renderwidget.h"

// Based on QCommandLinkButton, but this one scales with font size, supports richtext and can be
// associated with a QAction

class WelcomeButton : public QPushButton
{
    Q_OBJECT
    Q_DISABLE_COPY(WelcomeButton)
public:
    explicit WelcomeButton(QAction *a, QWidget *parent = nullptr);

    explicit WelcomeButton(QWidget *parent = nullptr)
        : WelcomeButton(QString(), QString(), parent)
    { }
    explicit WelcomeButton(const QString &text, QWidget *parent = nullptr)
        : WelcomeButton(text, QString(), parent)
    { }
    explicit WelcomeButton(const QString &text, const QString &description, QWidget *parent = nullptr);

    QString description() const
    {
        return m_description.text();
    }

    void setDescription(const QString &desc)
    {
        if (desc != m_description.text()) {
            m_description.setText(desc);
            updateGeometry();
            update();
        }
    }

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;
    int heightForWidth(int) const override;

protected:
    void changeEvent(QEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *) override;

private:
    void resetTitleFont()
    {
        m_titleFont = font();
        m_titleFont.setPointSizeF(m_titleFont.pointSizeF() * 1.5);
    }

    int textOffset() const;
    int descriptionOffset() const;
    QRect descriptionRect() const;
    QRect titleRect() const;
    int descriptionHeight(int widgetWidth) const;

    QFont m_titleFont;
    QStaticText m_description;
    int m_margin = 10;
};

WelcomeButton::WelcomeButton(QAction *a, QWidget *parent)
    : WelcomeButton(parent)
{
    if (!a)
        return;

    if (!a->icon().isNull()) {
        setIcon(a->icon());
    } else {
        const auto containers = a->associatedWidgets();
        for (auto *widget : containers) {
            if (QMenu *menu = qobject_cast<QMenu *>(widget)) {
                if (!menu->icon().isNull())
                    setIcon(menu->icon());
            }
        }
    }

    auto languageChange = [this](QAction *a) {
        setText(a->text());
        if (!a->shortcut().isEmpty()) {
            QString desc = "<i>(" + tr("Shortcut:") + " %1)</i>";
            setDescription(desc.arg(a->shortcut().toString()));
        }
        setToolTip(a->toolTip());
    };

    connect(this, &WelcomeButton::clicked, a, &QAction::trigger);
    connect(a, &QAction::changed, this, [languageChange, a]() { languageChange(a); });
    languageChange(a);
}

WelcomeButton::WelcomeButton(const QString &text, const QString &description, QWidget *parent)
    : QPushButton(text, parent)
    , m_description(description)
{
    setAttribute(Qt::WA_Hover);

    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred, QSizePolicy::PushButton);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);

    setIconSize({ 32, 32 });
    setIcon(QIcon(":/images/right_arrow"));

    resetTitleFont();
}

QRect WelcomeButton::titleRect() const
{
    QRect r = rect().adjusted(textOffset(), m_margin, -m_margin, 0);
    if (m_description.text().isEmpty()) {
        QFontMetrics fm(m_titleFont);
        r.setTop(r.top() + qMax(0, (icon().actualSize(iconSize()).height() - fm.height()) / 2));
    }
    return r;
}

QRect WelcomeButton::descriptionRect() const
{
    return rect().adjusted(textOffset(), descriptionOffset(), -m_margin, -m_margin);
}

int WelcomeButton::textOffset() const
{
    return m_margin + icon().actualSize(iconSize()).width() + m_margin;
}

int WelcomeButton::descriptionOffset() const
{
    QFontMetrics fm(m_titleFont);
    return m_margin + fm.height();
}

int WelcomeButton::descriptionHeight(int widgetWidth) const
{
    int lineWidth = widgetWidth - textOffset() - m_margin;
    QStaticText copy(m_description);
    copy.setTextWidth(lineWidth);
    return m_description.size().height();
}


QSize WelcomeButton::minimumSizeHint() const
{
    QSize size = sizeHint();
    int minimumHeight = qMax(descriptionOffset() + m_margin,
                             m_margin + icon().actualSize(iconSize()).height() + m_margin);
    size.setHeight(minimumHeight);
    return size;
}

QSize WelcomeButton::sizeHint() const
{
    QSize size = QPushButton::sizeHint();
    QFontMetrics fm(m_titleFont);
    int textWidth = qMax(fm.horizontalAdvance(text()), 135);
    int buttonWidth = m_margin + icon().actualSize(iconSize()).width() + m_margin + textWidth + m_margin;
    int heightWithoutDescription = descriptionOffset() + m_margin;

    size.setWidth(qMax(size.width(), buttonWidth));
    size.setHeight(qMax(m_description.text().isEmpty() ? 41 : 60,
                        heightWithoutDescription + descriptionHeight(buttonWidth)));
    return size;
}

int WelcomeButton::heightForWidth(int width) const
{
    int heightWithoutDescription = descriptionOffset() + m_margin;
    return qMax(heightWithoutDescription + descriptionHeight(width),
                m_margin + icon().actualSize(iconSize()).height() + m_margin);
}

void WelcomeButton::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::FontChange) {
        resetTitleFont();
        updateGeometry();
        update();
    }
    QWidget::changeEvent(e);
}

void WelcomeButton::resizeEvent(QResizeEvent *re)
{
    m_description.setTextWidth(titleRect().width());
    QWidget::resizeEvent(re);
}

void WelcomeButton::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    p.save();

    QStyleOptionButton option;
    initStyleOption(&option);

    //Enable command link appearance on Vista
    option.features |= QStyleOptionButton::CommandLinkButton;
    option.features &= ~QStyleOptionButton::Flat;
    option.text = QString();
    option.icon = QIcon(); //we draw this ourselves
    QSize pixmapSize = icon().actualSize(iconSize());

    const int vOffset = isDown()
        ? style()->pixelMetric(QStyle::PM_ButtonShiftVertical, &option) : 0;
    const int hOffset = isDown()
        ? style()->pixelMetric(QStyle::PM_ButtonShiftHorizontal, &option) : 0;

    //Draw icon
    p.drawControl(QStyle::CE_PushButton, option);
    if (!icon().isNull()) {
        QFontMetrics fm(m_titleFont);
        p.drawPixmap(m_margin + hOffset, m_margin + qMax(0, fm.height() - pixmapSize.height()) / 2 + vOffset,
                     icon().pixmap(pixmapSize,
                                   isEnabled() ? QIcon::Normal : QIcon::Disabled,
                                   isChecked() ? QIcon::On : QIcon::Off));
    }

    //Draw title
    int textflags = Qt::TextShowMnemonic;
    if (!style()->styleHint(QStyle::SH_UnderlineShortcut, &option, this))
        textflags |= Qt::TextHideMnemonic;

    p.setFont(m_titleFont);
    p.drawItemText(titleRect().translated(hOffset, vOffset),
                    textflags, option.palette, isEnabled(), text(), QPalette::ButtonText);

    //Draw description
    textflags |= Qt::TextWordWrap | Qt::ElideRight;
    p.setFont(font());
    p.drawStaticText(descriptionRect().translated(hOffset, vOffset).topLeft(), m_description);
//    p.drawItemText(descriptionRect().translated(hOffset, vOffset), textflags,
//                   option.palette, isEnabled(), description(), QPalette::ButtonText);
    p.restore();
}


WelcomeWidget::WelcomeWidget(QWidget *parent)
    : QWidget(parent)
{
    int spacing = style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
    int lmargin = style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    int rmargin = style()->pixelMetric(QStyle::PM_LayoutRightMargin);

    auto *layout = new QGridLayout();
    layout->setRowStretch(0, 10);
    layout->setRowStretch(4, 10);
    layout->setColumnStretch(0, 5);
    layout->setColumnStretch(1, 10);
    layout->setColumnStretch(2, 10);
    layout->setColumnStretch(3, 5);
    layout->setSpacing(2 * spacing);

    // info

    auto *info = new QWidget();
    auto info_layout = new QHBoxLayout();

    LDraw::Part *part = nullptr;

    if (LDraw::core())
        part = LDraw::core()->partFromId("3833");

    int iconSize = fontMetrics().height() * 4;

    if (part) {
        m_ldraw_icon = new LDraw::RenderOffscreenWidget();
        m_ldraw_icon->setPartAndColor(part, QColor("#F36100"));
        m_ldraw_icon->setFixedSize(iconSize, iconSize);
        m_ldraw_icon->startAnimation();
        info_layout->addWidget(m_ldraw_icon, 0, Qt::AlignCenter);
    } else {
        auto iconWidget = new QLabel();
        iconWidget->setPixmap(QPixmap(":/images/brickstore_icon.png"));
        iconWidget->setScaledContents(true);
        iconWidget->setFixedSize(iconSize, iconSize);
        info_layout->addWidget(iconWidget, 0, Qt::AlignCenter);
    }

    m_info_label = new QLabel();
    m_info_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    info_layout->addWidget(m_info_label, 1);

    info->setLayout(info_layout);
    layout->addWidget(info, 0, 1, 1, 2, Qt::AlignTop | Qt::AlignHCenter);

    // recent

    m_recent_frame = new QGroupBox();
    auto recent_layout = new QVBoxLayout();
    recent_layout->addStretch();
    m_recent_frame->setLayout(recent_layout);
    layout->addWidget(m_recent_frame, 1, 1, 2, 1);

    auto recreateRecentGroup = [this, recent_layout]() {
        while (recent_layout->count() > 1) {
            auto li = recent_layout->takeAt(0);
            delete li->widget();
            delete li;
        }

        auto recent = Config::inst()->recentFiles();
        if (recent.isEmpty()) {
            if (!m_no_recent)
                m_no_recent = new QLabel();
            recent_layout->insertWidget(0, m_no_recent);
        }

        int cnt = 0;
        for (const auto &f : recent) {
            auto b = new WelcomeButton(QFileInfo(f).fileName(), f);
            b->setIcon(QIcon(":/images/brickstore_doc_icon"));
            recent_layout->insertWidget(cnt++, b);
            connect(b, &WelcomeButton::clicked,
                    this, [b]() { FrameWork::inst()->openDocument(b->description()); });
        }
    };
    recreateRecentGroup();
    connect(Config::inst(), &Config::recentFilesChanged,
            this, recreateRecentGroup);

    // document

    m_file_frame = new QGroupBox();
    auto file_layout = new QVBoxLayout();
    for (const auto &name : { "file_new", "file_open" }) {
        auto b = new WelcomeButton(FrameWork::inst()->findAction(name));
        file_layout->addWidget(b);
    }
    m_file_frame->setLayout(file_layout);
    layout->addWidget(m_file_frame, 1, 2);

    // import

    m_import_frame = new QGroupBox();
    auto import_layout = new QVBoxLayout();
    for (const auto &name : { "file_import_bl_inv", "file_import_bl_xml", "file_import_bl_order",
         "file_import_bl_store_inv", "file_import_bl_cart", "file_import_ldraw_model" }) {
        auto b = new WelcomeButton(FrameWork::inst()->findAction(name));
        import_layout->addWidget(b);
    }
    import_layout->addStretch();
    m_import_frame->setLayout(import_layout);
    layout->addWidget(m_import_frame, 2, 2);

    // update

    m_update_frame = new QGroupBox();
    auto update_layout = new QHBoxLayout();
    update_layout->setSpacing(2 * spacing + lmargin + rmargin
                              + m_update_frame->contentsMargins().left()
                              + m_update_frame->contentsMargins().right());

    auto b = m_db_update = new WelcomeButton(FrameWork::inst()->findAction("extras_update_database"));
    update_layout->addWidget(b, 1);
    connect(Config::inst(), &Config::lastDatabaseUpdateChanged,
            this, &WelcomeWidget::updateLastDBUpdateDescription);
    auto dbLabelTimer = new QTimer(this);
    dbLabelTimer->setInterval(1000 * 60);
    dbLabelTimer->start();
    connect(dbLabelTimer, &QTimer::timeout,
            this, &WelcomeWidget::updateLastDBUpdateDescription);

    b = m_bs_update = new WelcomeButton(FrameWork::inst()->findAction("help_updates"));
    update_layout->addWidget(b, 1);

    m_update_frame->setLayout(update_layout);
    layout->addWidget(m_update_frame, 3, 1, 1, 2);

    languageChange();
    setLayout(layout);
}

void WelcomeWidget::updateLastDBUpdateDescription()
{
    auto delta = HumanReadableTimeDelta::toString(QDateTime::currentDateTime(),
                                                  Config::inst()->lastDatabaseUpdate());
    m_db_update->setDescription(tr("Last Database update: %1").arg(delta));
}

void WelcomeWidget::languageChange()
{
    m_recent_frame->setTitle(tr("Open recent files"));
    m_file_frame->setTitle(tr("Document"));
    m_import_frame->setTitle(tr("Import items"));
    m_update_frame->setTitle(tr("Updates"));

    m_bs_update->setDescription(tr("Current version: %1 (build: %2)")
                                .arg(BRICKSTORE_VERSION)
                                .arg(*BRICKSTORE_BUILD_NUMBER ? BRICKSTORE_BUILD_NUMBER : "custom"));
    updateLastDBUpdateDescription();

    QString infoText = QLatin1String("<strong style=\"font-size: x-large\">%1</strong><br>%2")
            .arg(QCoreApplication::applicationName())
            .arg(m_ldraw_icon ? tr("Using the LDraw installation at:")
                                + QLatin1String("<br><i>%1</i>").arg(LDraw::core()->dataPath())
                              : tr("No LDraw installation was found."));
    m_info_label->setText(infoText);

    if (m_no_recent)
        m_no_recent->setText(tr("No recent files"));
}

void WelcomeWidget::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
        languageChange();
    QWidget::changeEvent(e);
}


#include "welcomewidget.moc"
#include "moc_welcomewidget.cpp"
