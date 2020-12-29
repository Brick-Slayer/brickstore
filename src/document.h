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
#pragma once

#include <QAbstractTableModel>
#include <QDomDocument>
#include <QPixmap>
#include <QUuid>

#include "bricklink.h"
#include "filter.h"

class QUndoStack;
class UndoStack;
class QUndoCommand;
class AddRemoveCmd;
class ChangeCmd;


class Document : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Field {
        Status = 0,
        Picture,
        PartNo,
        Description,
        Condition,
        Color,
        Quantity,
        Price,
        Total,
        Bulk,
        Sale,
        Comments,
        Remarks,
        Category,
        ItemType,
        TierQ1,
        TierP1,
        TierQ2,
        TierP2,
        TierQ3,
        TierP3,
        LotId,
        Retain,
        Stockroom,
        Reserved,
        Weight,
        YearReleased,

        QuantityOrig,
        QuantityDiff,
        PriceOrig,
        PriceDiff,

        FieldCount
    };

    class Item : public BrickLink::InvItem
    {
    public:
        Item() = default;
        Item(const BrickLink::InvItem &);
        Item(const Item &) = default;
        ~Item() = default;

        Item &operator = (const Item &);
        bool operator == (const Item &) const;

        quint64 errors() const          { return m_errors; }
        void setErrors(quint64 errors) { m_errors = errors; }

        QImage image() const;

    private:
        quint64 m_errors = 0;

        friend class Document;
    };

    class ItemList : public QList<Item *>
    {
    public:
        ItemList() = default;
        ~ItemList() = default;
        ItemList(const ItemList &copy) : QList<Item *>(copy) { }
        ItemList(const BrickLink::InvItemList &iil)
        {
            for (const BrickLink::InvItem *ii : iil)
                append(new Item(*ii));
        }

        ItemList &operator=(const ItemList &other)
        {
            *static_cast<QList<Item *> *>(this) = static_cast<QList<Item *>>(other);
            return *this;
        }

        operator const BrickLink::InvItemList &() const
        {
            return reinterpret_cast<const BrickLink::InvItemList &>(*this);
        }
    };

    class Statistics
    {
    public:
        int lots() const             { return m_lots; }
        int items() const            { return m_items; }
        double value() const         { return m_val; }
        double minValue() const      { return m_minval; }
        double weight() const        { return m_weight; }
        int errors() const           { return m_errors; }
        int incomplete() const       { return m_incomplete; }
        QString currencyCode() const { return m_ccode; }

    private:
        friend class Document;

        Statistics(const Document *doc, const ItemList &list);

        int m_lots;
        int m_items;
        double m_val;
        double m_minval;
        double m_weight;
        int m_errors;
        int m_incomplete;
        QString m_ccode;
    };



    // Itemviews API
    Item *item(const QModelIndex &idx) const;
    QModelIndex index(const Item *i, int column = 0) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex&) const;
    bool setData(const QModelIndex&, const QVariant&, int);
    QVariant dataForEditRole(Item *it, Field f) const;
    QString dataForDisplayRole(Item *it, Field f) const;
    QVariant dataForDecorationRole(Item *it, Field f) const;
    Qt::CheckState dataForCheckStateRole(Item *it, Field f) const;
    int dataForTextAlignmentRole(Item *it, Field f) const;
    QString dataForToolTipRole(Item *it, Field f) const;
    static QString headerDataForDisplayRole(Field f);
    int headerDataForTextAlignmentRole(Field f) const;
    int headerDataForDefaultWidthRole(Field f) const;

    QString subConditionLabel(BrickLink::SubCondition sc) const;

public slots:
    void pictureUpdated(BrickLink::Picture *pic);

public:
    Document();
    virtual ~Document();

    static Document *createTemporary(const BrickLink::InvItemList &list);

    static const QList<Document *> &allDocuments();
    static QList<ItemList> restoreAutosave();

    QString fileName() const;
    QString title() const;

    const BrickLink::Order *order() const;

    bool isModified() const;

    const ItemList &items() const;
    bool clear();

    bool insertItems(const QVector<int> &positions, const ItemList &items);
    bool insertItem(int position, Item *item);

    bool removeItems(const ItemList &items);
    bool removeItem(Item *item);

    bool changeItem(int position, const Item &value);
    bool changeItem(Item *item, const Item &value);

    int positionOf(Item *item) const;
    const Item *itemAt(int position) const;

    void resetDifferences(const ItemList &items);

    Statistics statistics(const ItemList &list) const;

    quint64 errorMask() const;
    void setErrorMask(quint64);

    QString currencyCode() const;
    void setCurrencyCode(const QString &code, qreal crate = qreal(1));

    static Document *fileNew();
    static Document *fileOpen();
    static Document *fileOpen(const QString &name);
    static Document *fileImportBrickLinkInventory(const BrickLink::Item *preselect = nullptr);
    static QList<Document *> fileImportBrickLinkOrders();
    static Document *fileImportBrickLinkStore();
    static Document *fileImportBrickLinkCart();
    static Document *fileImportBrickLinkXML();
    static Document *fileImportLDrawModel();

public slots:
    void setFileName(const QString &str);
    void setTitle(const QString &str);

    void fileSave();
    void fileSaveAs();
    void fileExportBrickLinkXML(const Document::ItemList &itemlist);
    void fileExportBrickLinkXMLClipboard(const Document::ItemList &itemlist);
    void fileExportBrickLinkUpdateClipboard(const Document::ItemList &itemlist);
    void fileExportBrickLinkInvReqClipboard(const Document::ItemList &itemlist);
    void fileExportBrickLinkWantedListClipboard(const Document::ItemList &itemlist);

public:
    void beginMacro(const QString &label = QString());
    void endMacro(const QString &label = QString());

    QUndoStack *undoStack() const;

signals:
    void itemsAdded(const Document::ItemList &);
    void itemsAboutToBeRemoved(const Document::ItemList &);
    void itemsRemoved(const Document::ItemList &);
    void itemsChanged(const Document::ItemList &, bool);

    void errorsChanged(Document::Item *);
    void statisticsChanged();
    void fileNameChanged(const QString &);
    void titleChanged(const QString &);
    void modificationChanged(bool);
    void currencyCodeChanged(const QString &ccode);

private slots:
    void clean2Modified(bool);
    void autosave() const;

private:
    Document(int dummy);
    void deleteAutosave();
    static Document *fileLoadFrom(const QString &s, const char *type, bool import_only = false);
    bool fileSaveTo(const QString &s, const char *type, bool export_only, const ItemList &itemlist);
    void setBrickLinkItems(const BrickLink::InvItemList &bllist, uint multiply = 1);

    void insertItemsDirect(ItemList &items, QVector<int> &positions);
    void removeItemsDirect(ItemList &items, QVector<int> &positions);
    void changeItemDirect(int position, Item &item);
    void changeCurrencyDirect(const QString &ccode, qreal crate, double *&prices);

    friend class AddRemoveCmd;
    friend class ChangeCmd;
    friend class CurrencyCmd;

    void updateErrors(Item *);

private:
    ItemList         m_items;

    QString          m_currencycode;
    quint64          m_error_mask = 0;
    QString          m_filename;
    QString          m_title;
    QUuid            m_uuid;  // for autosave
    QTimer           m_autosave_timer;

    UndoStack *      m_undo = nullptr;

    BrickLink::Order *m_order = nullptr;

    static QList<Document *> s_documents;
};


class DocumentProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    DocumentProxyModel(Document *model, QObject *parent = nullptr);
    ~DocumentProxyModel();

    inline Document::Item *item(const QModelIndex &idx) const  { return static_cast<Document *>(sourceModel())->item(mapToSource(idx)); }
    inline QModelIndex index(const Document::Item *i) const  { return mapFromSource(static_cast<Document *>(sourceModel())->index(i)); }
    using QSortFilterProxyModel::index;

    Document::ItemList sortItemList(const Document::ItemList &list) const;

    void setFilterExpression(const QString &filter);
    QString filterExpression() const;
    QString filterToolTip() const;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool event(QEvent *e) override;

private:
    void languageChange();
    static int compare(const Document::Item *i1, const Document::Item *i2, int sortColumn);

    QString         m_filter_expression;
    Filter::Parser *m_parser;
    QList<Filter>   m_filter;

    int             m_lastSortColumn[2];

    friend class SortItemListCompare;
};

QDataStream &operator << (QDataStream &ds, const Document::Item &item);
QDataStream &operator >> (QDataStream &ds, Document::Item &item);