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
#include <QTimer>
#include <QEvent>
#include <QDir>
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>
#include <QSysInfo>
#include <QFileOpenEvent>
#include <QProcess>
#include <QDesktopServices>
#include <QLocalSocket>
#include <QLocalServer>
#include <QNetworkProxyFactory>

#if defined(Q_OS_WINDOWS)
#  include <QStyleFactory>
#  include <windows.h>
#  ifdef MessageBox
#    undef MessageBox
#  endif
#  include <wininet.h>
#elif defined(Q_OS_MACOS)
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <SystemConfiguration/SCNetworkReachability.h>
#elif defined(Q_OS_UNIX)
#  include <sys/utsname.h>
#endif

#include "informationdialog.h"
#include "progressdialog.h"
#include "checkforupdates.h"
#include "config.h"
#include "rebuilddatabase.h"
#include "bricklink.h"
#include "ldraw.h"
#include "messagebox.h"
#include "framework.h"
#include "transfer.h"
#include "report.h"
#include "currency.h"

#include "utility.h"
#include "version.h"
#include "application.h"
#include "stopwatch.h"

#define XSTR(a) #a
#define STR(a) XSTR(a)

enum {
#if defined(QT_NO_DEBUG)
    isDeveloperBuild = 0,
#else
    isDeveloperBuild = 1,
#endif
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    isUnix = 1,
#else
    isUnix = 0,
#endif
    is64Bit = (sizeof(void *) / 8)
};

Application *Application::s_inst = nullptr;

Application::Application(bool rebuild_db_only, bool skip_download, int &_argc, char **_argv)
    : QObject()
{
    s_inst = this;

    QCoreApplication::setApplicationName(QLatin1String(BRICKSTORE_NAME));
    QCoreApplication::setApplicationVersion(QLatin1String(BRICKSTORE_VERSION));

    QCoreApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);

#if !defined(Q_OS_WINDOWS) // HighDPI work fine, but only without this setting
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#  if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#  endif
#endif

    if (rebuild_db_only) {
        new QCoreApplication(_argc, _argv);
    } else {
#if defined(Q_OS_WINDOWS)
        if (auto s = QStyleFactory::create("fusion"))
            QApplication::setStyle(s);
#endif
        new QApplication(_argc, _argv);

        qApp->installEventFilter(this);

        m_default_fontsize = QGuiApplication::font().pointSizeF();
        qApp->setProperty("_bs_defaultFontSize", m_default_fontsize); // the settings dialog needs this

        auto setFontSizePercentLambda = [this](int p) {
            QFont f = QApplication::font();
            f.setPointSizeF(m_default_fontsize * qreal(qBound(50, p, 200)) / 100.);
            QApplication::setFont(f);
        };
        connect(Config::inst(), &Config::fontSizePercentChanged, this, setFontSizePercentLambda);
        int fsp = Config::inst()->fontSizePercent();
        if (fsp != 100)
            setFontSizePercentLambda(fsp);

#if !defined(Q_OS_WINDOWS) && !defined(Q_OS_MACOS)
        QPixmap pix(":/images/brickstore_icon.png");
        if (!pix.isNull())
            QGuiApplication::setWindowIcon(pix);
#endif
#if defined(Q_OS_MACOS)
        QGuiApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

        // check for an already running instance
        if (isClient()) {
            // we cannot call quit directly, since there is
            // no event loop to quit from...
            QMetaObject::invokeMethod(this, &QCoreApplication::quit, Qt::QueuedConnection);
            return;
        }
    }

    checkNetwork();
    auto *netcheck = new QTimer(this);
    connect(netcheck, &QTimer::timeout,
            this, &Application::checkNetwork);
    netcheck->start(5000);

    QNetworkProxyFactory::setUseSystemConfiguration(true);

//    Transfer::setDefaultUserAgent(applicationName() + QLatin1Char('/') + applicationVersion() +
//                                  QLatin1String(" (") + systemName() + QLatin1Char(' ') + systemVersion() +
//                                  QLatin1String("; http://") + applicationUrl() + QLatin1Char(')'));

    //TODO5: find out why we are blacklisted ... for now, fake the UA
    Transfer::setDefaultUserAgent("Br1ckstore" + QLatin1Char('/') + QCoreApplication::applicationVersion() +
                                  QLatin1String(" (") + QSysInfo::prettyProductName() +
                                  QLatin1Char(')'));


    // initialize config & resource
    (void) Config::inst()->upgrade(BRICKSTORE_MAJOR, BRICKSTORE_MINOR);
    (void) Currency::inst();
    (void) ReportManager::inst();

    if (!initBrickLink()) {
        // we cannot call quit directly, since there is no event loop to quit from...
        QMetaObject::invokeMethod(qApp, &QCoreApplication::quit, Qt::QueuedConnection);
        return;

    } else if (rebuild_db_only) {
        QMetaObject::invokeMethod(this, [skip_download]() {
            RebuildDatabase rdb(skip_download);
            exit(rdb.exec());
        }, Qt::QueuedConnection);
    }
    else {
        updateTranslations();
        connect(Config::inst(), &Config::languageChanged,
                this, &Application::updateTranslations);

        MessageBox::setDefaultTitle(QCoreApplication::applicationName());

        m_files_to_open << QCoreApplication::arguments().mid(1);

        FrameWork::inst()->show();
#if defined(Q_OS_MACOS)
        FrameWork::inst()->raise();
#endif
    }
}

Application::~Application()
{
    exitBrickLink();

    delete ReportManager::inst();
    delete Currency::inst();
    delete Config::inst();

    delete qApp;
}

QStringList Application::externalResourceSearchPath(const QString &subdir) const
{
    static QStringList baseSearchPath;

    if (baseSearchPath.isEmpty()) {
        QString appdir = QCoreApplication::applicationDirPath();
#if defined(Q_OS_WINDOWS)
        baseSearchPath << appdir;

        if (isDeveloperBuild)
            baseSearchPath << appdir + QLatin1String("/..");
#elif defined(Q_OS_MACOS)
        baseSearchPath << appdir + QLatin1String("/../Resources");
#elif defined(Q_OS_UNIX)
        baseSearchPath << QLatin1String(STR(INSTALL_PREFIX) "/share/brickstore");

        if (isDeveloperBuild)
            baseSearchPath << appdir;
#endif
    }
    if (subdir.isEmpty()) {
        return baseSearchPath;
    } else {
        QStringList searchPath;
        for (const QString &bsp : qAsConst(baseSearchPath))
            searchPath << bsp + QDir::separator() + subdir;
        return searchPath;
    }
}

void Application::updateTranslations()
{
    QString locale = Config::inst()->language();
    if (locale.isEmpty())
        locale = QLocale::system().name();
    QLocale::setDefault(QLocale(locale));

    if (m_trans_qt)
        QCoreApplication::removeTranslator(m_trans_qt.data());
    if (m_trans_brickstore)
        QCoreApplication::removeTranslator(m_trans_brickstore.data());
    m_trans_qt.reset(new QTranslator());
    m_trans_brickstore.reset(new QTranslator());
    if (!m_trans_brickstore_en)
        m_trans_brickstore_en.reset(new QTranslator());

    QString i18n = ":/i18n";

    static bool once = false; // always load the english plural forms
    if (!once) {
        if (m_trans_brickstore_en->load("brickstore_en", i18n))
            QCoreApplication::installTranslator(m_trans_brickstore_en.data());
        once = true;
    }

    if (locale != "en") {
        if (m_trans_qt->load(QLatin1String("qtbase_") + locale, i18n))
            QCoreApplication::installTranslator(m_trans_qt.data());
        else
            m_trans_qt.reset();

        if (m_trans_brickstore->load(QLatin1String("brickstore_") + locale, i18n))
            QCoreApplication::installTranslator(m_trans_brickstore.data());
        else
            m_trans_brickstore.reset();
    }
}

QString Application::applicationUrl() const
{
    return QLatin1String(BRICKSTORE_URL);
}

void Application::enableEmitOpenDocument(bool b)
{
    if (b != m_enable_emit) {
        m_enable_emit = b;

        if (b && !m_files_to_open.isEmpty())
            QTimer::singleShot(0, this, SLOT(doEmitOpenDocument()));
    }
}

void Application::doEmitOpenDocument()
{
    while (m_enable_emit && !m_files_to_open.isEmpty())
        emit openDocument(m_files_to_open.takeFirst());
}

bool Application::eventFilter(QObject *o, QEvent *e)
{
    if ((o != qApp) || !e)
        return false;

    switch (e->type()) {
    case QEvent::FileOpen:
        m_files_to_open.append(static_cast<QFileOpenEvent *>(e)->file());
        doEmitOpenDocument();
        return true;
    default:
        return QObject::eventFilter(o, e);
    }
}

bool Application::isClient(int timeout)
{
    enum { Undecided, Server, Client } state = Undecided;
    QString socketName = QLatin1String("BrickStore");
    QLocalServer *server = nullptr;

#if defined(Q_OS_WINDOWS)
    // QLocalServer::listen() would succeed for any number of callers
    HANDLE semaphore = ::CreateSemaphore(nullptr, 0, 1, L"Local\\BrickStore");
    state = (semaphore && (::GetLastError() == ERROR_ALREADY_EXISTS)) ? Client : Server;
#endif
    if (state != Client) {
        server = new QLocalServer(this);

        bool res = server->listen(socketName);
#if defined(Q_OS_UNIX)
        if (!res && server->serverError() == QAbstractSocket::AddressInUseError) {
            QFile::remove(QDir::cleanPath(QDir::tempPath()) + QLatin1Char('/') + socketName);
            res = server->listen(socketName);
        }
#endif
        if (res) {
            connect(server, &QLocalServer::newConnection,
                    this, &Application::clientMessage);
            state = Server;
        }
    }
    if (state != Server) {
        QLocalSocket client(this);

        for (int i = 0; i < 2; ++i) { // try twice
            client.connectToServer(socketName);
            if (client.waitForConnected(timeout / 2) || i)
                break;
            else
                QThread::sleep(timeout / 4);
        }

        if (client.state() == QLocalSocket::ConnectedState) {
            QStringList files;
            foreach (const QString &arg, QCoreApplication::arguments().mid(1)) {
                QFileInfo fi(arg);
                if (fi.exists() && fi.isFile())
                    files << fi.absoluteFilePath();
            }
            QByteArray data;
            QDataStream ds(&data, QIODevice::WriteOnly);
            ds << qint32(0) << files;
            ds.device()->seek(0);
            ds << qint32(data.size() - int(sizeof(qint32)));

            bool res = (client.write(data) == data.size());
            res = res && client.waitForBytesWritten(timeout / 2);
            res = res && client.waitForReadyRead(timeout / 2);
            res = res && (client.read(1) == QByteArray(1, 'X'));

            if (res) {
                delete server;
                return true;
            }
        }
    }
    return false;
}

void Application::clientMessage()
{
    auto *server = qobject_cast<QLocalServer *>(sender());
    if (!server)
        return;
    QLocalSocket *client = server->nextPendingConnection();
    if (!client)
        return;

    QDataStream ds(client);
    QStringList files;
    bool header = true;
    int need = sizeof(qint32);
    while (need) {
        auto got = client->bytesAvailable();
        if (got < need) {
            client->waitForReadyRead();
        } else if (header) {
            need = 0;
            ds >> need;
            header = false;
        } else {
            ds >> files;
            need = 0;
        }
    }
    client->write("X", 1);

    m_files_to_open += files;
    doEmitOpenDocument();

    if (FrameWork::inst()) {
        FrameWork::inst()->setWindowState(FrameWork::inst()->windowState() & ~Qt::WindowMinimized);
        FrameWork::inst()->raise();
        FrameWork::inst()->activateWindow();
    }
}

bool Application::initBrickLink()
{
    QString errstring;

    BrickLink::Core *bl = BrickLink::create(Config::inst()->dataDir(), &errstring);
    if (!bl) {
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(), tr("Could not initialize the BrickLink kernel:<br /><br />%1").arg(errstring), QMessageBox::Ok);
    } else {
        bl->setItemImageScaleFactor(Config::inst()->itemImageSizePercent() / 100.);
        connect(Config::inst(), &Config::itemImageSizePercentChanged,
                this, [](qreal p) { BrickLink::core()->setItemImageScaleFactor(p / 100.); });
        bl->setTransfer(new Transfer);

        connect(Config::inst(), &Config::updateIntervalsChanged,
                BrickLink::core(), &BrickLink::Core::setUpdateIntervals);
        BrickLink::core()->setUpdateIntervals(Config::inst()->updateIntervals());
    }
    LDraw::create(QString(), &errstring);

    return bl;
}


void Application::exitBrickLink()
{
    delete BrickLink::core();
    delete LDraw::core();
}


void Application::about()
{
    QString layout = QLatin1String(
        "<center>"
        "<table border=\"0\"><tr>"
        "<td valign=\"middle\" align=\"center\" width=\"168\">"
        "<img src=\":/images/brickstore_icon.png\" width=\"128\" style=\"margin: 20\"/></td>"
        "<td align=\"left\">"
        "<strong style=\"font-size: x-large\">%1</strong><br>"
        "<strong style=\"font-size: large\">%3</strong><br>"
        "<span style=\"font-size: large\">%2</strong><br>"
        "<br>%4</td>"
        "</tr></table>"
        "</center><center>"
        "<br><big>%5</big>"
        "</center>%6<p>%7</p>");


    QString page1_link = QLatin1String("<strong>%1</strong> | <a href=\"system\">%2</a>");
    page1_link = page1_link.arg(tr("Legal Info"), tr("System Info"));
    QString page2_link = QLatin1String("<a href=\"index\">%1</a> | <strong>%2</strong>");
    page2_link = page2_link.arg(tr("Legal Info"), tr("System Info"));

    QString copyright = tr("Copyright &copy; %1").arg(BRICKSTORE_COPYRIGHT);
    QString version   = tr("Version %1 (build: %2)").arg(BRICKSTORE_VERSION)
            .arg(*BRICKSTORE_BUILD_NUMBER ? BRICKSTORE_BUILD_NUMBER : "custom");
    QString support   = tr("Visit %1").arg("<a href=\"https://" BRICKSTORE_URL "\">" BRICKSTORE_URL "</a>");

    QString qt = QLibraryInfo::version().toString();
    if (QLibraryInfo::isDebugBuild())
        qt += QLatin1String(" (debug build)");

    QString translators = QLatin1String("<b>") + tr("Translators") + QLatin1String("</b><table border=\"0\">");

    QString translators_html = QLatin1String(R"(<tr><td>%1</td><td width="2em"></td><td>%2 &lt;<a href="mailto:%3">%4</a>&gt;</td></tr>)");
    const auto translations = Config::inst()->translations();
    for (const Config::Translation &trans : translations) {
        if (trans.language != QLatin1String("en")) {
            QString langname = trans.languageName.value(QLocale().name().left(2), trans.languageName[QLatin1String("en")]);
            translators += translators_html.arg(langname, trans.author, trans.authorEMail, trans.authorEMail);
        }
    }

    translators += QLatin1String("</table>");

    QString legal = tr(
        "<p>"
        "This program is free software; it may be distributed and/or modified "
        "under the terms of the GNU General Public License version 2 as published "
        "by the Free Software Foundation and appearing in the file LICENSE.GPL "
        "included in this software package."
        "<br>"
        "This program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE "
        "WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE."
        "<br>"
        "See <a href=\"http://fsf.org/licensing/licenses/gpl.html\">www.fsf.org/licensing/licenses/gpl.html</a> for GPL licensing information."
        "</p><p>"
        "All data from <a href=\"https://www.bricklink.com\">www.bricklink.com</a> is owned by BrickLink<sup>TM</sup>, "
        "which is a trademark of Dan Jezek."
        "</p><p>"
        "LEGO<sup>&reg;</sup> is a trademark of the LEGO group of companies, "
        "which does not sponsor, authorize or endorse this software."
        "</p><p>"
        "All other trademarks recognised."
        "</p>");

    QString technical = QLatin1String(
        "<table>"
        "<th colspan=\"2\" align=\"left\">Build Info</th>"
        "<tr><td>Git version   </td><td>%1</td></tr>"
        "<tr><td>User          </td><td>%2</td></tr>"
        "<tr><td>Host          </td><td>%3</td></tr>"
        "<tr><td>Date          </td><td>%4</td></tr>"
        "<tr><td>Architecture  </td><td>%5</td></tr>"
        "<tr><td>Compiler      </td><td>%6</td></tr>"
        "</table><br>"
        "<table>"
        "<th colspan=\"2\" align=\"left\">Runtime Info</th>"
        "<tr><td>OS            </td><td>%7</td></tr>"
        "<tr><td>Architecture  </td><td>%8</td></tr>"
        "<tr><td>Memory        </td><td>%L9 MB</td></tr>"
        "<tr><td>Qt            </td><td>%10</td></tr>"
        "</table>");

    technical = technical.arg(BRICKSTORE_GIT_VERSION, BRICKSTORE_BUILD_USER, BRICKSTORE_BUILD_HOST, __DATE__ " " __TIME__)
            .arg(QSysInfo::buildCpuArchitecture()).arg(BRICKSTORE_COMPILER_VERSION)
            .arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture())
            .arg(Utility::physicalMemory()/(1024ULL*1024ULL)).arg(qt);

    QString page1 = layout.arg(QCoreApplication::applicationName(), copyright, version, support, page1_link, legal, translators);
    QString page2 = layout.arg(QCoreApplication::applicationName(), copyright, version, support, page2_link, technical, QString());

    QMap<QString, QString> pages;
    pages ["index"]  = page1;
    pages ["system"] = page2;

    InformationDialog d(QCoreApplication::applicationName(), pages, FrameWork::inst());
    d.exec();
}

void Application::checkForUpdates()
{
    Transfer trans;

    ProgressDialog d(&trans, FrameWork::inst());
    CheckForUpdates cfu(&d);
    d.exec();
}

#define CHECK_IP "178.63.92.134" // brickforge.de

void Application::checkNetwork()
{
    bool online = false;

#if defined(Q_OS_LINUX)
    int res = ::system("ip route get " CHECK_IP "/32 >/dev/null 2>/dev/null");

    //qWarning() << "Linux NET change: " << res << WIFEXITED(res) << WEXITSTATUS(res);
    if (!WIFEXITED(res) || (WEXITSTATUS(res) == 0 || WEXITSTATUS(res) == 127))
        online = true;

#elif defined(Q_OS_MACOS)
    static SCNetworkReachabilityRef target = nullptr;

    if (!target) {
        struct ::sockaddr_in sock;
        sock.sin_family = AF_INET;
        sock.sin_port = htons(80);
        sock.sin_addr.s_addr = inet_addr(CHECK_IP);

        target = SCNetworkReachabilityCreateWithAddress(nullptr, reinterpret_cast<sockaddr *>(&sock));
        // in theory we should CFRelease(target) when exitting
    }

    SCNetworkReachabilityFlags flags = 0;
    if (!target || !SCNetworkReachabilityGetFlags(target, &flags)
            || ((flags & (kSCNetworkReachabilityFlagsReachable | kSCNetworkReachabilityFlagsConnectionRequired))
                         == kSCNetworkReachabilityFlagsReachable)) {
        online = true;
    }

#elif defined(Q_OS_WINDOWS)
    // this function is buggy/unreliable
    //online = InternetCheckConnectionW(L"http://" TEXT(CHECK_IP), 0, 0);
    DWORD flags;
    online = InternetGetConnectedStateEx(&flags, nullptr, 0, 0);
    //qWarning() << "Win NET change: " << online;

#else
    online = true;
#endif

    if (online != m_online) {
        m_online = online;
        emit onlineStateChanged(m_online);
    }
}

bool Application::isOnline() const
{
    return m_online;
}

#include "moc_application.cpp"