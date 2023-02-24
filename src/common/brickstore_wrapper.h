// Copyright (C) 2004-2023 Robert Griebl
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtQml/QQmlParserStatus>
#include <QtQml/QJSValue>
#include <QPointer>
#include <QIdentityProxyModel>
#include <QSortFilterProxyModel>
#include <QItemSelectionModel>
#include <QClipboard>
#include <QFont>

#include <QCoro/QCoroQmlTask>

#include "common/application.h"
#include "common/currency.h"
#include "common/onlinestate.h"
#include "common/recentfiles.h"
#include "common/config.h"
#include "common/announcements.h"
#include "common/systeminfo.h"

#include "common/document.h"
#include "common/documentlist.h"

#include <bricklink/qmlapi.h>
#include "utility/utility.h"

class QmlColor;
class QmlCategory;
class QmlItemType;
class QmlItem;
class QmlLot;
class QmlPriceGuide;
class QmlPicture;
class QmlDocumentColumnModel;


class QmlDocument : public QAbstractProxyModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Document)
    QML_UNCREATABLE("")
    QML_EXTENDED_NAMESPACE(DocumentModel)
    Q_PRIVATE_PROPERTY(model(), bool sorted READ isSorted NOTIFY isSortedChanged FINAL)
    Q_PRIVATE_PROPERTY(model(), bool filtered READ isFiltered NOTIFY isFilteredChanged FINAL)
    Q_PRIVATE_PROPERTY(model(), QString currencyCode READ currencyCode NOTIFY currencyCodeChanged FINAL)
    Q_PRIVATE_PROPERTY(model(), int lotCount READ lotCount NOTIFY lotCountChanged FINAL)
    Q_PRIVATE_PROPERTY(model(), bool modified READ isModified NOTIFY modificationChanged FINAL)
    Q_PRIVATE_PROPERTY(doc(), QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PRIVATE_PROPERTY(doc(), QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged FINAL)
    Q_PRIVATE_PROPERTY(doc(), QString fileName READ fileName NOTIFY fileNameChanged FINAL)
    Q_PRIVATE_PROPERTY(doc(), QImage thumbnail READ thumbnail WRITE setThumbnail NOTIFY thumbnailChanged FINAL)
    Q_PRIVATE_PROPERTY(doc(), BrickLink::Order *order READ order NOTIFY orderChanged FINAL)
    Q_PRIVATE_PROPERTY(doc(), bool blockingOperationActive READ isBlockingOperationActive NOTIFY blockingOperationActiveChanged FINAL)
    Q_PRIVATE_PROPERTY(doc(), bool blockingOperationCancelable READ isBlockingOperationCancelable NOTIFY blockingOperationCancelableChanged FINAL)
    Q_PRIVATE_PROPERTY(doc(), QString blockingOperationTitle READ blockingOperationTitle WRITE setBlockingOperationTitle NOTIFY blockingOperationTitleChanged FINAL)
    Q_PRIVATE_PROPERTY(doc(), bool restoredFromAutosave READ isRestoredFromAutosave CONSTANT FINAL)
    Q_PRIVATE_PROPERTY(doc(), QItemSelectionModel *selectionModel READ selectionModel CONSTANT FINAL)

    Q_PROPERTY(QVariantList sortColumns READ qmlSortColumns NOTIFY qmlSortColumnsChanged FINAL)
    Q_PROPERTY(QmlDocumentLots *lots READ qmlLots CONSTANT FINAL)
    Q_PROPERTY(QList<BrickLink::QmlLot> selectedLots READ qmlSelectedLots NOTIFY qmlSelectedLotsChanged FINAL)
    Q_PROPERTY(QmlDocumentColumnModel *columnModel READ columnModel CONSTANT FINAL)
    Q_PROPERTY(Document *document READ document CONSTANT FINAL)

public:
    QmlDocument(Document *doc);

    int rowCount(const QModelIndex &parent = { }) const override;
    int columnCount(const QModelIndex &parent = { }) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = { }) const override;
    QModelIndex parent(const QModelIndex &child) const override;

    QVariant headerData(int section, Qt::Orientation o, int role) const override;

    QModelIndex mapToSource(const QModelIndex &idx) const override;
    QModelIndex mapFromSource(const QModelIndex &sindex) const override;

    Q_INVOKABLE int logicalColumn(int visual) const;
    Q_INVOKABLE int visualColumn(int logical) const;

    QmlDocumentColumnModel *columnModel();

    QVariantList qmlSortColumns() const;
    QmlDocumentLots *qmlLots();
    QList<BrickLink::QmlLot> qmlSelectedLots();

    Q_INVOKABLE void sort(int column, Qt::SortOrder order) override;
    Q_INVOKABLE void sortAdditionally(int column, Qt::SortOrder order);

    Q_INVOKABLE DocumentStatistics selectionStatistics(bool ignoreExcluded = false) const;

    Q_INVOKABLE void saveCurrentColumnLayout();
    Q_INVOKABLE void setColumnLayoutFromId(const QString &layoutId);

    Q_INVOKABLE void cancelBlockingOperation();

    Q_INVOKABLE void setPriceToGuide(BrickLink::Time time, BrickLink::Price price, bool forceUpdate);
    Q_INVOKABLE void setColor(BrickLink::QmlColor color);

    Q_INVOKABLE void priceAdjust(bool isFixed, double value, bool applyToTiers);
    Q_INVOKABLE void costAdjust(bool isFixed, double value);

signals:
    void forceLayout();

    void closeAllViewsForDocument();
    void requestActivation();

    void isSortedChanged(bool b);
    void isFilteredChanged(bool b);
    void currencyCodeChanged(const QString &ccode);
    void lotCountChanged(int lotCount);
    void modificationChanged(bool modified);
    void filePathChanged(const QString &filePath);
    void fileNameChanged(const QString &fileName);
    void titleChanged(const QString &title);
    void thumbnailChanged(const QImage &image);
    void orderChanged(BrickLink::Order *order);
    void blockingOperationActiveChanged(bool blocked);
    void blockingOperationCancelableChanged(bool cancelable);
    void blockingOperationTitleChanged(const QString &title);
    void blockingOperationProgress(int done, int total);

    void qmlSortColumnsChanged();
    void qmlSelectedLotsChanged();

private:
    DocumentModel *model() { return m_doc->model(); }
    Document *doc() { return m_doc; }

    void setDocument(Document *doc);
    Document *document() const;

    void update();
    void emitForceLayout();

    void internalHideColumn(int vi, bool visible);
    void internalMoveColumn(int viFrom, int viTo);
    bool internalIsColumnHidden(int li) const;

    QVector<int> l2v;
    QVector<int> v2l;

    QPointer<Document> m_doc;
    QObject *m_connectionContext = nullptr;
    QTimer *m_forceLayoutDelay = nullptr;
    QmlDocumentColumnModel *m_columnModel = nullptr;

    QPointer<QmlDocumentLots> m_qmlLots;

    friend class QmlDocumentColumnModel;
};




class QmlDocumentColumnModel : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(DocumentColumnModel)
    QML_ANONYMOUS
    Q_PROPERTY(int count READ rowCount CONSTANT FINAL)

public:
    QmlDocumentColumnModel(QmlDocument *proxyModel);

    int rowCount(const QModelIndex &parent = { }) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void moveColumn(int viFrom, int viTo);
    Q_INVOKABLE void hideColumn(int vi, bool hidden);

    Q_INVOKABLE bool isColumnHidden(int li) const;

private:
    QmlDocument *m_proxyModel;
    friend class QmlDocument;
};


class ColumnLayoutsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ColumnLayoutModel)
    QML_UNCREATABLE("")

public:
    ColumnLayoutsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = { }) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

private:
    void update();

    QVector<QPair<QString, QString>> m_idAndName;
};


class QmlSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SortFilterProxyModel)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(int sortColumn READ sortColumn WRITE setSortColumn NOTIFY sortColumnChanged FINAL)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged FINAL)

    Q_PROPERTY(QString sortRoleName READ sortRoleName WRITE setSortRoleName NOTIFY sortRoleNameChanged FINAL)
    Q_PROPERTY(QString filterRoleName READ filterRoleName WRITE setFilterRoleName NOTIFY filterRoleNameChanged FINAL)
    Q_PROPERTY(QString filterString READ filterString WRITE setFilterString FINAL)
    Q_PROPERTY(FilterSyntax filterSyntax READ filterSyntax WRITE setFilterSyntax NOTIFY filterSyntaxChanged FINAL)

public:
    enum FilterSyntax {
        RegularExpression,
        Wildcard,
        FixedString
    };
    Q_ENUM(FilterSyntax)

    QmlSortFilterProxyModel(QObject *parent = nullptr);

    int count() const;

    QString sortRoleName() const;
    void setSortRoleName(const QString &role);

    QString filterRoleName() const;
    void setFilterRoleName(const QString &role);

    void setSortColumn(int newSortColumn);
    void setSortOrder(Qt::SortOrder newSortOrder);

    QString filterString() const;
    void setFilterString(const QString &filter);

    FilterSyntax filterSyntax() const;
    void setFilterSyntax(FilterSyntax syntax);

    QHash<int, QByteArray> roleNames() const override;

signals:
    void countChanged(int newCount);
    void sortOrderChanged(int newSortOrder);
    void sortColumnChanged(int newSortColumn);

    void sortRoleNameChanged(QString newSortRoleName);
    void filterRoleNameChanged(QString newFilterRoleName);

    void filterSyntaxChanged(QmlSortFilterProxyModel::FilterSyntax newFilterSyntax);

private:
    int roleKey(const QByteArray &role) const;

    FilterSyntax m_filterSyntax = FixedString;
};


class QmlClipboard : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Clipboard)
    QML_SINGLETON

public:
    QmlClipboard() = default;

    Q_INVOKABLE void clear();
    Q_INVOKABLE QString text() const;
    Q_INVOKABLE void setText(const QString &text);
};

class QmlUtility : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Utility)
    QML_SINGLETON

public:
    QmlUtility() = default;

    Q_INVOKABLE bool fuzzyCompare(double d1, double d2) { return Utility::fuzzyCompare(d1, d2); }

    Q_INVOKABLE int naturalCompare(const QString &s1, const QString &s2) const { return Utility::naturalCompare(s1, s2); }

    Q_INVOKABLE QColor gradientColor(const QColor &c1, const QColor &c2, float f = 0.5) { return Utility::gradientColor(c1, c2, f); }
    Q_INVOKABLE QColor textColor(const QColor &backgroundColor) { return Utility::textColor(backgroundColor); }
    Q_INVOKABLE QColor contrastColor(const QColor &c, float f) { return Utility::contrastColor(c, f); }
    Q_INVOKABLE QColor shadeColor(int n, float alpha) { return Utility::shadeColor(n, alpha); }

    Q_INVOKABLE QString weightToString(double gramm, QLocale::MeasurementSystem ms, bool optimize = false, bool show_unit = false) { return Utility::weightToString(gramm, ms, optimize, show_unit); }
    Q_INVOKABLE double stringToWeight(const QString &s, QLocale::MeasurementSystem ms) { return Utility::stringToWeight(s, ms); }

    Q_INVOKABLE double roundTo(double f, int decimals) { return Utility::roundTo(f, decimals); }
};


class QmlDebugLogModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    static QmlDebugLogModel *inst();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void append(QtMsgType type, const QString &category, const QString &file, int line,
                const QString &message);

private:
    QmlDebugLogModel(QObject *parent = nullptr);
    static QmlDebugLogModel *s_inst;

    struct Log {
        QtMsgType type;
        int line;
        QString category;
        QString file;
        QString message;
    };
    std::vector<Log> m_logs;
};

class QmlDebug : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Debug)
    QML_UNCREATABLE("")
    Q_PROPERTY(bool showTracers READ showTracers WRITE setShowTracers NOTIFY showTracersChanged FINAL)
    Q_PROPERTY(QAbstractListModel *log READ log CONSTANT FINAL)

public:
    QmlDebug(QObject *parent = nullptr);

    bool showTracers() const;
    void setShowTracers(bool newShowTracers);
    QAbstractListModel *log() const;

signals:
    void showTracersChanged(bool newShowTracers);

private:
    bool m_showTracers;
};

class QmlAnnouncements
{
    Q_GADGET
    QML_FOREIGN(Announcements)
    QML_NAMED_ELEMENT(Announcements)
    QML_SINGLETON

public:
     static Announcements *create(QQmlEngine *, QJSEngine *)
     {
         auto a = Application::inst()->announcements();
         QQmlEngine::setObjectOwnership(a, QQmlEngine::CppOwnership);
         return a;
     }
};

class QmlCurrency
{
    Q_GADGET
    QML_FOREIGN(Currency)
    QML_NAMED_ELEMENT(Currency)
    QML_SINGLETON

public:
     static Currency *create(QQmlEngine *, QJSEngine *)
     {
         auto c = Currency::inst();
         QQmlEngine::setObjectOwnership(c, QQmlEngine::CppOwnership);
         return c;
     }
};


class QmlSystemInfo
{
    Q_GADGET
    QML_FOREIGN(SystemInfo)
    QML_NAMED_ELEMENT(SystemInfo)
    QML_SINGLETON

public:
     static SystemInfo *create(QQmlEngine *, QJSEngine *)
     {
         auto si = SystemInfo::inst();
         QQmlEngine::setObjectOwnership(si, QQmlEngine::CppOwnership);
         return si;
     }
};

class QmlConfig
{
    Q_GADGET
    QML_FOREIGN(Config)
    QML_NAMED_ELEMENT(Config)
    QML_SINGLETON

public:
     static Config *create(QQmlEngine *, QJSEngine *)
     {
         auto c = Config::inst();
         QQmlEngine::setObjectOwnership(c, QQmlEngine::CppOwnership);
         return c;
     }
};


class QmlOnlineState
{
    Q_GADGET
    QML_FOREIGN(OnlineState)
    QML_NAMED_ELEMENT(OnlineState)
    QML_SINGLETON

public:
     static OnlineState *create(QQmlEngine *, QJSEngine *)
     {
         auto os = OnlineState::inst();
         QQmlEngine::setObjectOwnership(os, QQmlEngine::CppOwnership);
         return os;
     }
};

class QmlRecentFiles
{
    Q_GADGET
    QML_FOREIGN(RecentFiles)
    QML_NAMED_ELEMENT(RecentFiles)
    QML_UNCREATABLE("")
};


class QmlDocumentLots : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    QmlDocumentLots(DocumentModel *model);

    Q_INVOKABLE int add(BrickLink::QmlItem item, BrickLink::QmlColor color);
    Q_INVOKABLE void remove(BrickLink::QmlLot lot);
    Q_INVOKABLE void removeAt(int index);
    Q_INVOKABLE BrickLink::QmlLot at(int index);

private:
    DocumentModel *m_model;

    friend class BrickLink::QmlLot::Setter;
};

class QmlLotList
{
    Q_GADGET
    QML_FOREIGN(QList<BrickLink::QmlLot>)
    QML_ANONYMOUS
    QML_SEQUENTIAL_CONTAINER(BrickLink::QmlLot)
};



class QmlDocumentList : public QIdentityProxyModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(DocumentList)
    QML_UNCREATABLE("")
    Q_PRIVATE_PROPERTY(docList(), int count READ count NOTIFY countChanged FINAL)

public:
    QmlDocumentList(QObject *parent = nullptr);

    QmlDocument *map(Document *doc) const;

    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QmlDocument *document(int index) const;

signals:
    void lastDocumentClosed();
    void countChanged(int count);
    void documentAdded(QmlDocument *document);
    void documentRemoved(QmlDocument *document);

private:
    DocumentList *docList();
    QHash<Document *, QmlDocument *> m_docMapping;
};



class QmlBrickStore : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(BrickStore)
    QML_SINGLETON
    Q_PROPERTY(QString defaultCurrencyCode READ defaultCurrencyCode NOTIFY defaultCurrencyCodeChanged FINAL)
    Q_PROPERTY(QString versionNumber READ versionNumber CONSTANT FINAL)
    Q_PROPERTY(QString buildNumber READ buildNumber CONSTANT FINAL)
    Q_PROPERTY(RecentFiles *recentFiles READ recentFiles CONSTANT FINAL)
    Q_PROPERTY(QmlDocumentList *documents READ documents CONSTANT FINAL)
    Q_PROPERTY(QmlDocument *activeDocument READ activeDocument NOTIFY activeDocumentChanged FINAL)
    Q_PROPERTY(ColumnLayoutsModel *columnLayouts READ columnLayouts CONSTANT FINAL)
    Q_PROPERTY(QVariantMap about READ about CONSTANT FINAL)
    Q_PROPERTY(QmlDebug *debug READ debug CONSTANT FINAL)

public:
    static QmlBrickStore *inst();
    static QmlBrickStore *create(QQmlEngine *, QJSEngine *);

    QmlDocumentList *documents() const;
    Config *config() const;
    QString versionNumber() const;
    QString buildNumber() const;
    RecentFiles *recentFiles() const;
    ColumnLayoutsModel *columnLayouts() const;
    QVariantMap about() const;
    QmlDebug *debug() const;
    QString defaultCurrencyCode() const;

    Q_INVOKABLE QString symbolForCurrencyCode(const QString &currencyCode) const;
    Q_INVOKABLE QString toCurrencyString(double value, const QString &symbol = { }, int precision = 3) const;
    Q_INVOKABLE QString toWeightString(double value, bool showUnit = false) const;

    Q_INVOKABLE QStringList nameFiltersForBrickLinkXML(bool includeAll = false) const;
    Q_INVOKABLE QStringList nameFiltersForBrickStoreXML(bool includeAll = false) const;
    Q_INVOKABLE QStringList nameFiltersForLDraw(bool includeAll = false) const;

    Q_INVOKABLE void importBrickLinkStore(BrickLink::Store *store);
    Q_INVOKABLE void importBrickLinkOrder(BrickLink::Order *order);
    Q_INVOKABLE void importBrickLinkCart(BrickLink::Cart *cart);

    Q_INVOKABLE void importPartInventory(BrickLink::QmlItem item, BrickLink::QmlColor color,
                                         int multiply, BrickLink::Condition condition,
                                         BrickLink::Status extraParts,
                                         BrickLink::PartOutTraits partOutTraits,
                                         BrickLink::Status status = BrickLink::Status::Include);

    Q_INVOKABLE void updateDatabase();

    QmlDocument *activeDocument() const;

    Q_INVOKABLE QCoro::QmlTask checkBrickLinkLogin();

    Q_INVOKABLE double maxLocalPrice(const QString &currencyCode);

    Q_INVOKABLE QString cacheStats() const;

signals:
    void defaultCurrencyCodeChanged(const QString &defaultCurrencyCode);
    void showSettings(const QString &page);
    void activeDocumentChanged(QmlDocument *doc);

private:
    QmlBrickStore();
    static QmlBrickStore *s_inst;
    QmlDocumentList *m_docList;
    ColumnLayoutsModel *m_columnLayouts;
    mutable QmlDebug *m_debug = nullptr;
};

