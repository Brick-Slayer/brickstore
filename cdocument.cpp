/* Copyright (C) 2004-2008 Robert Griebl. All rights reserved.
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
#include <float.h>

#include <qapplication.h>
#include <qcursor.h>
#include <qfiledialog.h>
#include <qclipboard.h>
#include <qprinter.h>
#include <qpainter.h>
#include <qregexp.h>
#include <QDesktopServices>

#if defined( MODELTEST )
#include <modeltest.h>
#define MODELTEST_ATTACH(x)   { (void) new ModelTest(x, x); }
#else
#define MODELTEST_ATTACH(x)   ;
#endif

#include "cutility.h"
#include "cconfig.h"
#include "cframework.h"
#include "cmessagebox.h"
//#include "creport.h"

#include "dimportorder.h"
#include "dimportinventory.h"
//#include "dlgselectreportimpl.h"
//#include "dlgincompleteitemimpl.h"

#include "cimport.h"

#include "cdocument.h"
#include "cdocument_p.h"

#include "clocalemeasurement.h"

namespace {

template <typename T> static inline T pack(typename T::const_reference item)
{
    T list;
    list.append(item);
    return list;
}

enum {
    CID_Change,
    CID_AddRemove
};

}


CChangeCmd::CChangeCmd(CDocument *doc, CDocument::Item *pos, const CDocument::Item &item, bool merge_allowed)
    : QUndoCommand(qApp->translate("CChangeCmd", "Modified item")), m_doc(doc), m_position(pos), m_item(item), m_merge_allowed(merge_allowed)
{ }

CChangeCmd::~CChangeCmd()
{ }

int CChangeCmd::id() const
{
    return CID_Change;
}

void CChangeCmd::redo()
{
    m_doc->changeItemDirect(m_position, m_item);
}

void CChangeCmd::undo()
{
    redo();
}

bool CChangeCmd::mergeWith(const QUndoCommand *other)
{
    const CChangeCmd *that = static_cast <const CChangeCmd *>(other);

    if ((m_merge_allowed && that->m_merge_allowed) &&
        (m_doc == that->m_doc) &&
        (m_position == that->m_position))
    {
        return true;
    }
    return false;
}


CAddRemoveCmd::CAddRemoveCmd(Type t, CDocument *doc, const CDocument::ItemList &positions, const CDocument::ItemList &items, bool merge_allowed)
    : QUndoCommand(genDesc(t == Add, qMax(items.count(), positions.count()))),
      m_doc(doc), m_positions(positions), m_items(items), m_type(t), m_merge_allowed(merge_allowed)
{ }

CAddRemoveCmd::~CAddRemoveCmd()
{
    if (m_type == Add)
        qDeleteAll(m_items);
}

int CAddRemoveCmd::id() const
{
    return CID_AddRemove;
}

void CAddRemoveCmd::redo()
{
    if (m_type == Add) {
        // CDocument::insertItemsDirect() needs to update the itlist iterators to point directly to the items!
        m_doc->insertItemsDirect(m_items, m_positions);
        m_positions.clear();
        m_type = Remove;
    }
    else {
        // CDocument::removeItems() needs to update the itlist iterators to point directly past the items
        //                                    and fill the m_item_list with the items
        m_doc->removeItemsDirect(m_items, m_positions);
        m_type = Add;
    }
}

void CAddRemoveCmd::undo()
{
    redo();
}

bool CAddRemoveCmd::mergeWith(const QUndoCommand *other)
{
    const CAddRemoveCmd *that = static_cast <const CAddRemoveCmd *>(other);

    if ((m_merge_allowed && that->m_merge_allowed) &&
        (m_doc == that->m_doc) &&
        (m_type == that->m_type)) {
        m_items     += that->m_items;
        m_positions += that->m_positions;
        setText(genDesc(m_type == Remove, qMax(m_items.count(), m_positions.count())));

        const_cast<CAddRemoveCmd *>(that)->m_items.clear();
        const_cast<CAddRemoveCmd *>(that)->m_positions.clear();
        return true;
    }
    return false;
}

QString CAddRemoveCmd::genDesc(bool is_add, uint count)
{
    if (is_add)
        return CDocument::tr("Added %n item(s)").arg(count);
    else
        return CDocument::tr("Removed %n item(s)").arg(count);
}


// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************

CDocument::Statistics::Statistics(const CDocument *doc, const ItemList &list)
{
    m_lots = list.count();
    m_items = 0;
    m_val = m_minval = 0.;
    m_weight = .0;
    m_errors = 0;
    bool weight_missing = false;

    foreach(const Item *item, list) {
        int qty = item->quantity();
        money_t price = item->price();

        m_val += (qty * price);

        for (int i = 0; i < 3; i++) {
            if (item->tierQuantity(i) && (item->tierPrice(i) != 0))
                price = item->tierPrice(i);
        }
        m_minval += (qty * price * (1.0 - double(item->sale()) / 100.0));
        m_items += qty;

        if (item->weight() > 0)
            m_weight += item->weight();
        else
            weight_missing = true;

        if (item->errors()) {
            quint64 errors = item->errors() & doc->m_error_mask;

            for (uint i = 1ULL << (FieldCount - 1); i;  i >>= 1) {
                if (errors & i)
                    m_errors++;
            }
        }
    }
    if (weight_missing)
        m_weight = (m_weight == 0.) ? -DBL_MIN : -m_weight;
}


// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************


CDocument::Item::Item()
    : BrickLink::InvItem(0, 0), m_errors(0)
{ }

CDocument::Item::Item(const BrickLink::InvItem &copy)
    : BrickLink::InvItem(copy), m_errors(0)
{ }

CDocument::Item::Item(const Item &copy)
    : BrickLink::InvItem(copy), m_errors(copy.m_errors)
{ }

CDocument::Item &CDocument::Item::operator = (const Item &copy)
{
    BrickLink::InvItem::operator = (copy);

    m_errors = copy.m_errors;
    return *this;
}

CDocument::Item::~Item()
{ }

bool CDocument::Item::operator == (const Item &cmp) const
{
    // ignore errors for now!
    return BrickLink::InvItem::operator == (cmp);
}

QImage CDocument::Item::image() const
{
    BrickLink::Picture *pic = BrickLink::core()->picture(item(), color());

    if (pic && pic->valid()) {
        return pic->image();
    }
    else {
        QSize s = BrickLink::core()->pictureSize(item()->itemType());
        QImage img(s, QImage::Format_Mono);
        img.fill(Qt::white);
        return img;
    }
}

QPixmap CDocument::Item::pixmap() const
{
    return QPixmap::fromImage(image());
}

// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************

QList<CDocument *> CDocument::s_documents;

CDocument::CDocument(bool dont_sort)
{
    MODELTEST_ATTACH(this)

    m_undo = new QUndoStack(this);
    m_order = 0;
    m_error_mask = 0;
    m_dont_sort = dont_sort;

    m_selection_model = new QItemSelectionModel(this, this);
    connect(m_selection_model, SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(selectionHelper()));

    connect(BrickLink::core(), SIGNAL(pictureUpdated(BrickLink::Picture *)), this, SLOT(pictureUpdated(BrickLink::Picture *)));

    connect(m_undo, SIGNAL(cleanChanged(bool)), this, SLOT(clean2Modified(bool)));

    s_documents.append(this);
}

CDocument::~CDocument()
{
    delete m_order;
    qDeleteAll(m_items);

    s_documents.removeAll(this);
}

void CDocument::selectionHelper()
{
    m_selection.clear();

    foreach (const QModelIndex &idx, m_selection_model->selectedRows())
        m_selection.append(item(idx));

    emit selectionChanged(m_selection);
}

const CDocument::ItemList &CDocument::selection() const
{
    return m_selection;
}

void CDocument::setSelection(const CDocument::ItemList &lst)
{
    QItemSelection idxs;

    foreach (const Item *item, lst) {
        QModelIndex idx(index(item));
        idxs.select(idx, idx);
    }
    m_selection_model->select(idxs, QItemSelectionModel::Clear | QItemSelectionModel::Select | QItemSelectionModel::Current |QItemSelectionModel::Rows);
}


QItemSelectionModel *CDocument::selectionModel() const
{
    return m_selection_model;
}

const QList<CDocument *> &CDocument::allDocuments()
{
    return s_documents;
}

bool CDocument::doNotSortItems() const
{
    return m_dont_sort;
}

const CDocument::ItemList &CDocument::items() const
{
    return m_items;
}

CDocument::Statistics CDocument::statistics(const ItemList &list) const
{
    return Statistics(this, list);
}

void CDocument::beginMacro(const QString &label)
{
    m_undo->beginMacro(label);
}

void CDocument::endMacro(const QString &)
{
    //TODO: Fix Qt to accept a label in QUndoStack::endMacro()

    m_undo->endMacro(/*label*/);
}

QUndoStack *CDocument::undoStack() const
{
    return m_undo;
}


bool CDocument::clear()
{
    m_undo->push(new CAddRemoveCmd(CAddRemoveCmd::Remove, this, ItemList(), m_items));
    return true;
}

bool CDocument::insertItems(const ItemList &positions, const ItemList &items)
{
    m_undo->push(new CAddRemoveCmd(CAddRemoveCmd::Add, this, positions, items /*, true*/));
    return true;
}

bool CDocument::removeItems(const ItemList &positions)
{
    m_undo->push(new CAddRemoveCmd(CAddRemoveCmd::Remove, this, ItemList(), positions /*, true*/));
    return true;
}

bool CDocument::insertItem(Item *position, Item *item)
{
    return insertItems(pack<ItemList> (position), pack<ItemList> (item));
}

bool CDocument::removeItem(Item *position)
{
    return removeItems(pack<ItemList> (position));
}

bool CDocument::changeItem(Item *position, const Item &item)
{
    if (!(item == *position))
        m_undo->push(new CChangeCmd(this, position, item /*, true*/));
    return true;
}

void CDocument::insertItemsDirect(ItemList &items, ItemList &positions)
{
    ItemList::iterator pos = positions.begin();
    QModelIndex root;

    foreach(Item *item, items) {
        if (pos != positions.end()) {
            int idx;
            if (Item *itempos = *pos)
                idx = m_items.indexOf(itempos);
            else
                idx = rowCount(root);
            emit beginInsertRows(root, idx, idx);

            m_items.insert(idx, item);
            ++pos;
        }
        else {
            emit beginInsertRows(root, rowCount(root), rowCount(root));
            m_items.append(item);
        }
        updateErrors(item);

        emit endInsertRows();
    }

    emit itemsAdded(items);
    emit statisticsChanged();
}

void CDocument::removeItemsDirect(ItemList &items, ItemList &positions)
{
    positions.clear();

    emit itemsAboutToBeRemoved(items);

    foreach(Item *item, items) {
        int idx = m_items.indexOf(item);
        emit beginRemoveRows(QModelIndex(), idx, idx);

        ItemList::iterator next = m_items.erase(qFind(m_items.begin(), m_items.end(), item));
        positions.append((next != m_items.end()) ? *next : 0);

        emit endRemoveRows();
    }

    emit itemsRemoved(items);
    emit statisticsChanged();
}

void CDocument::changeItemDirect(Item *position, Item &item)
{
    qSwap(*position, item);

    bool grave = (position->item() != item.item()) || (position->color() != item.color());

    emit itemsChanged(pack<ItemList> (position), grave);
    updateErrors(position);
    emit statisticsChanged();

    QModelIndex idx1 = index(position);
    QModelIndex idx2 = createIndex(idx1.row(), columnCount(idx1.parent()) - 1, idx1.internalPointer());
    emit dataChanged(idx1, idx2);
}

void CDocument::updateErrors(Item *item)
{
    quint64 errors = 0;

    if (item->price() <= 0)
        errors |= (1ULL << Price);

    if (item->quantity() <= 0)
        errors |= (1ULL << Quantity);

    if ((item->color()->id() != 0) && (!item->itemType()->hasColors()))
        errors |= (1ULL << Color);

    if (item->tierQuantity(0) && ((item->tierPrice(0) <= 0) || (item->tierPrice(0) >= item->price())))
        errors |= (1ULL << TierP1);

    if (item->tierQuantity(1) && ((item->tierPrice(1) <= 0) || (item->tierPrice(1) >= item->tierPrice(0))))
        errors |= (1ULL << TierP2);

    if (item->tierQuantity(1) && (item->tierQuantity(1) <= item->tierQuantity(0)))
        errors |= (1ULL << TierQ2);

    if (item->tierQuantity(2) && ((item->tierPrice(2) <= 0) || (item->tierPrice(2) >= item->tierPrice(1))))
        errors |= (1ULL << TierP3);

    if (item->tierQuantity(2) && (item->tierQuantity(2) <= item->tierQuantity(1)))
        errors |= (1ULL << TierQ3);

    if (errors != item->errors()) {
        item->setErrors(errors);
        emit errorsChanged(item);
        emit statisticsChanged();
    }
}


CDocument *CDocument::fileNew()
{
    CDocument *doc = new CDocument();
    doc->setTitle(tr("Untitled"));
    return doc;
}

CDocument *CDocument::fileOpen()
{
    QStringList filters;
    filters << tr("Inventory Files") + " (*.bsx *.bti)";
    filters << tr("BrickStore XML Data") + " (*.bsx)";
    filters << tr("BrikTrak Inventory") + " (*.bti)";
    filters << tr("All Files") + "(*.*)";

    return fileOpen(QFileDialog::getOpenFileName(CFrameWork::inst(), tr("Open File"), CConfig::inst()->documentDir(), filters.join(";;")));
}

CDocument *CDocument::fileOpen(const QString &s)
{
    if (s.isEmpty())
        return 0;

    QString abs_s  = QFileInfo(s).absoluteFilePath();

    foreach(CDocument *doc, s_documents) {
        if (QFileInfo(doc->fileName()).absoluteFilePath() == abs_s)
            return doc;
    }

    CDocument *doc = 0;

    if (s.right(4) == ".bti") {
        doc = fileImportBrikTrakInventory(s);

        if (doc)
            CMessageBox::information(CFrameWork::inst(), tr("BrickStore has switched to a new file format (.bsx - BrickStore XML).<br /><br />Your document has been automatically imported and it will be converted as soon as you save it."));
    }
    else
        doc = fileLoadFrom(s, "bsx");

    return doc;
}

CDocument *CDocument::fileImportBrickLinkInventory(const BrickLink::Item *preselect)
{
    DImportInventory dlg(CFrameWork::inst());

    if (preselect)
        dlg.setItem(preselect);

    if (dlg.exec() == QDialog::Accepted) {
        const BrickLink::Item *it = dlg.item();
        int qty = dlg.quantity();

        if (it && (qty > 0)) {
            BrickLink::InvItemList items = it->consistsOf();

            if (!items.isEmpty()) {
                CDocument *doc = new CDocument(true);

                doc->setBrickLinkItems(items, qty);
                doc->setTitle(tr("Inventory for %1").arg(it->id()));
                return doc;
            }
            else
                CMessageBox::warning(CFrameWork::inst(), tr("Internal error: Could not create an Inventory object for item %1").arg(CMB_BOLD(it->id())));
        }
        else
            CMessageBox::warning(CFrameWork::inst(), tr("Requested item was not found in the database."));
    }
    return 0;
}

QList<CDocument *> CDocument::fileImportBrickLinkOrders()
{
    QList<CDocument *> docs;

    DImportOrder dlg(CFrameWork::inst());

    if (dlg.exec() == QDialog::Accepted) {
        QList<QPair<BrickLink::Order *, BrickLink::InvItemList *> > orders = dlg.orders();

        for (QList<QPair<BrickLink::Order *, BrickLink::InvItemList *> >::const_iterator it = orders.begin(); it != orders.end(); ++it) {
            const QPair<BrickLink::Order *, BrickLink::InvItemList *> &order = *it;

            if (order.first && order.second) {
                CDocument *doc = new CDocument(true);

                doc->setTitle(tr("Order #%1").arg(order.first->id()));
                doc->setBrickLinkItems(*order.second);
                doc->m_order = new BrickLink::Order(*order. first);
                docs.append(doc);
            }
        }
    }
    return docs;
}

CDocument *CDocument::fileImportBrickLinkStore()
{
    CProgressDialog d(CFrameWork::inst());
    CImportBLStore import(&d);

    if (d.exec() == QDialog::Accepted) {
        CDocument *doc = new CDocument();

        doc->setTitle(tr("Store %1").arg(QDate::currentDate().toString(Qt::LocalDate)));
        doc->setBrickLinkItems(import.items());
        return doc;
    }
    return 0;
}

CDocument *CDocument::fileImportBrickLinkCart()
{
    QString url = QApplication::clipboard()->text(QClipboard::Clipboard);
    QRegExp rx_valid("http://www\\.bricklink\\.com/storeCart\\.asp\\?h=[0-9]+&b=[0-9]+");

    if (!rx_valid.exactMatch(url))
        url = "http://www.bricklink.com/storeCart.asp?h=______&b=______";

    if (CMessageBox::getString(CFrameWork::inst(), tr("Enter the URL of your current BrickLink shopping cart:"
                               "<br /><br />Right-click on the <b>View Cart</b> button "
                               "in your browser and copy the URL to the clipboard by choosing "
                               "<b>Copy Link Location</b> (Firefox), <b>Copy Link</b> (Safari) "
                               "or <b>Copy Shortcut</b> (Internet Explorer).<br /><br />"
                               "<em>Super-lots and custom items are <b>not</b> supported</em>."), url)) {
        QRegExp rx("\\?h=([0-9]+)&b=([0-9]+)");
        rx.indexIn(url);
        int shopid = rx.cap(1).toInt();
        int cartid = rx.cap(2).toInt();

        if (shopid && cartid) {
            CProgressDialog d(CFrameWork::inst());
            CImportBLCart import(shopid, cartid, &d);

            if (d.exec() == QDialog::Accepted) {
                CDocument *doc = new CDocument(true);

                doc->setBrickLinkItems(import.items());
                doc->setTitle(tr("Cart in Shop %1").arg(shopid));
                return doc;
            }
        }
        else
            QApplication::beep();
    }
    return 0;
}

CDocument *CDocument::fileImportBrickLinkXML()
{
    QStringList filters;
    filters << tr("BrickLink XML File") + " (*.xml)";
    filters << tr("All Files") + "(*.*)";

    QString s = QFileDialog::getOpenFileName(CFrameWork::inst(), tr("Import File"), CConfig::inst()->documentDir(), filters.join(";;"));

    if (!s.isEmpty()) {
        CDocument *doc = fileLoadFrom(s, "xml", true);

        if (doc)
            doc->setTitle(tr("Import of %1").arg(QFileInfo(s).fileName()));
        return doc;
    }
    else
        return 0;
}

CDocument *CDocument::fileImportPeeronInventory()
{
    QString peeronid;

    if (CMessageBox::getString(CFrameWork::inst(), tr("Enter the set ID of the Peeron inventory:"), peeronid)) {
        CProgressDialog d(CFrameWork::inst());
        CImportPeeronInventory import(peeronid, &d);

        if (d.exec() == QDialog::Accepted) {
            CDocument *doc = new CDocument(true);

            doc->setBrickLinkItems(import.items());
            doc->setTitle(tr("Peeron Inventory for %1").arg(peeronid));
            return doc;
        }
    }
    return 0;
}

CDocument *CDocument::fileImportBrikTrakInventory(const QString &fn)
{
    QString s = fn;

    if (s.isNull()) {
        QStringList filters;
        filters << tr("BrikTrak Inventory") + " (*.bti)";
        filters << tr("All Files") + "(*.*)";

        s = QFileDialog::getOpenFileName(CFrameWork::inst(), tr("Import File"), CConfig::inst()->documentDir(), filters.join(";;"));
    }

    if (!s.isEmpty()) {
        CDocument *doc = fileLoadFrom(s, "bti", true);

        if (doc)
            doc->setTitle(tr("Import of %1").arg(QFileInfo(s).fileName()));
        return doc;
    }
    else
        return 0;
}

CDocument *CDocument::fileLoadFrom(const QString &name, const char *type, bool import_only)
{
    BrickLink::ItemListXMLHint hint;

    if (qstrcmp(type, "bsx") == 0)
        hint = BrickLink::XMLHint_BrickStore;
    else if (qstrcmp(type, "bti") == 0)
        hint = BrickLink::XMLHint_BrikTrak;
    else if (qstrcmp(type, "xml") == 0)
        hint = BrickLink::XMLHint_MassUpload;
    else
        return false;


    QFile f(name);

    if (!f.open(QIODevice::ReadOnly)) {
        CMessageBox::warning(CFrameWork::inst(), tr("Could not open file %1 for reading.").arg(CMB_BOLD(name)));
        return false;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    uint invalid_items = 0;
    BrickLink::InvItemList *items = 0;

    QString emsg;
    int eline = 0, ecol = 0;
    QDomDocument doc;

    if (doc.setContent(&f, &emsg, &eline, &ecol)) {
        QDomElement root = doc.documentElement();
        QDomElement item_elem;

        if (hint == BrickLink::XMLHint_BrickStore) {
            for (QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling()) {
                if (!n.isElement())
                    continue;

                if (n.nodeName() == "Inventory")
                    item_elem = n.toElement();
            }
        }
        else {
            item_elem = root;
        }

        items = BrickLink::core()->parseItemListXML(item_elem, hint, &invalid_items);
    }
    else {
        CMessageBox::warning(CFrameWork::inst(), tr("Could not parse the XML data in file %1:<br /><i>Line %2, column %3: %4</i>").arg(CMB_BOLD(name)).arg(eline).arg(ecol).arg(emsg));
        QApplication::restoreOverrideCursor();
        return false;
    }

    QApplication::restoreOverrideCursor();

    if (items) {
        CDocument *doc = new CDocument(import_only);

        if (invalid_items)
            CMessageBox::information(CFrameWork::inst(), tr("This file contains %1 unknown item(s).").arg(CMB_BOLD(QString::number(invalid_items))));

        doc->setBrickLinkItems(*items);
        delete items;

        doc->setFileName(import_only ? QString::null : name);

        if (!import_only)
            CFrameWork::inst()->addToRecentFiles(name);

        return doc;
    }
    else {
        CMessageBox::warning(CFrameWork::inst(), tr("Could not parse the XML data in file %1.").arg(CMB_BOLD(name)));
        return false;
    }
}

CDocument *CDocument::fileImportLDrawModel()
{
    QStringList filters;
    filters << tr("LDraw Models") + " (*.dat;*.ldr;*.mpd)";
    filters << tr("All Files") + "(*.*)";

    QString s = QFileDialog::getOpenFileName(CFrameWork::inst(), tr("Import File"), CConfig::inst()->documentDir(), filters.join(";;"));

    if (s.isEmpty())
        return false;

    QFile f(s);

    if (!f.open(QIODevice::ReadOnly)) {
        CMessageBox::warning(CFrameWork::inst(), tr("Could not open file %1 for reading.").arg(CMB_BOLD(s)));
        return false;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    uint invalid_items = 0;
    BrickLink::InvItemList items;

    bool b = BrickLink::core()->parseLDrawModel(f, items, &invalid_items);

    QApplication::restoreOverrideCursor();

    if (b && !items.isEmpty()) {
        CDocument *doc = new CDocument(true);

        if (invalid_items)
            CMessageBox::information(CFrameWork::inst(), tr("This file contains %1 unknown item(s).").arg(CMB_BOLD(QString::number(invalid_items))));

        doc->setBrickLinkItems(items);
        doc->setTitle(tr("Import of %1").arg(QFileInfo(s).fileName()));
        return doc;
    }
    else
        CMessageBox::warning(CFrameWork::inst(), tr("Could not parse the LDraw model in file %1.").arg(CMB_BOLD(s)));

    return 0;
}

void CDocument::setBrickLinkItems(const BrickLink::InvItemList &bllist, uint multiply)
{
    ItemList items;
    ItemList positions;

    foreach(const BrickLink::InvItem *blitem, bllist) {
        Item *item = new Item(*blitem);

        if (item->isIncomplete()) {
//   DlgIncompleteItemImpl d ( item, /*CFrameWork::inst ( )*/ 0);

//-   if ( waitcursor )
//-    QApplication::restoreOverrideCursor ( );

            bool drop_this = true; //( d.exec ( ) != QDialog::Accepted );

//-   if ( waitcursor )
//-    QApplication::setOverrideCursor ( QCursor( Qt::WaitCursor ));

            if (drop_this)
                continue;
        }

        item->setQuantity(item->quantity() * multiply);
        items.append(item);
    }
    insertItemsDirect(items, positions);

    // reset difference WITHOUT a command

    foreach(Item *pos, m_items) {
        if ((pos->origQuantity() != pos->quantity()) ||
            (pos->origPrice() != pos->price()))
        {
            pos->setOrigQuantity(pos->quantity());
            pos->setOrigPrice(pos->price());
        }
    }
}

QString CDocument::fileName() const
{
    return m_filename;
}

void CDocument::setFileName(const QString &str)
{
    m_filename = str;

    QFileInfo fi(str);

    if (fi.exists())
        setTitle(QDir::convertSeparators(fi.absoluteFilePath()));

    emit fileNameChanged(m_filename);
}

QString CDocument::title() const
{
    return m_title;
}

void CDocument::setTitle(const QString &str)
{
    m_title = str;
    emit titleChanged(m_title);
}

bool CDocument::isModified() const
{
    return !m_undo->isClean();
}

void CDocument::fileSave(const ItemList &itemlist)
{
    if (fileName().isEmpty())
        fileSaveAs(itemlist);
    else if (isModified())
        fileSaveTo(fileName(), "bsx", false, itemlist);
}


void CDocument::fileSaveAs(const ItemList &itemlist)
{
    QStringList filters;
    filters << tr("BrickStore XML Data") + " (*.bsx)";

    QString fn = fileName();

    if (fn.isEmpty()) {
        QDir d(CConfig::inst()->documentDir());

        if (d.exists())
            fn = d.filePath(m_title);
    }
    if ((fn.right(4) == ".xml") || (fn.right(4) == ".bti"))
        fn.truncate(fn.length() - 4);

    fn = QFileDialog::getSaveFileName(CFrameWork::inst(), tr("Save File as"), fn, filters.join(";;"));

    if (!fn.isNull()) {
        if (fn.right(4) != ".bsx")
            fn += ".bsx";

        if (QFile::exists(fn) &&
            CMessageBox::question(CFrameWork::inst(), tr("A file named %1 already exists.Are you sure you want to overwrite it?").arg(CMB_BOLD(fn)), CMessageBox::Yes, CMessageBox::No) != CMessageBox::Yes)
            return;

        fileSaveTo(fn, "bsx", false, itemlist);
    }
}


bool CDocument::fileSaveTo(const QString &s, const char *type, bool export_only, const ItemList &itemlist)
{
    BrickLink::ItemListXMLHint hint;

    if (qstrcmp(type, "bsx") == 0)
        hint = BrickLink::XMLHint_BrickStore;
    else if (qstrcmp(type, "bti") == 0)
        hint = BrickLink::XMLHint_BrikTrak;
    else if (qstrcmp(type, "xml") == 0)
        hint = BrickLink::XMLHint_MassUpload;
    else
        return false;


    QFile f(s);
    if (f.open(QIODevice::WriteOnly)) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        QDomDocument doc((hint == BrickLink::XMLHint_BrickStore) ? QString("BrickStoreXML") : QString::null);
        doc.appendChild(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));

        QDomElement item_elem = BrickLink::core()->createItemListXML(doc, hint, reinterpret_cast<const BrickLink::InvItemList *>(&itemlist));

        if (hint == BrickLink::XMLHint_BrickStore) {
            QDomElement root = doc.createElement("BrickStoreXML");

            root.appendChild(item_elem);
            doc.appendChild(root);
        }
        else {
            doc.appendChild(item_elem);
        }

        // directly writing to an QTextStream would be way more efficient,
        // but we could not handle any error this way :(
        QByteArray output = doc.toByteArray();
        bool ok = (f.write(output.data(), output.size() - 1) ==  qint64(output.size() - 1));             // no 0-byte

        QApplication::restoreOverrideCursor();

        if (ok) {
            if (!export_only) {
                m_undo->setClean();
                setFileName(s);

                CFrameWork::inst()->addToRecentFiles(s);
            }
            return true;
        }
        else
            CMessageBox::warning(CFrameWork::inst(), tr("Failed to save data in file %1.").arg(CMB_BOLD(s)));
    }
    else
        CMessageBox::warning(CFrameWork::inst(), tr("Failed to open file %1 for writing.").arg(CMB_BOLD(s)));

    return false;
}

void CDocument::fileExportBrickLinkInvReqClipboard(const ItemList &itemlist)
{
    QDomDocument doc(QString::null);
    doc.appendChild(BrickLink::core()->createItemListXML(doc, BrickLink::XMLHint_Inventory, reinterpret_cast<const BrickLink::InvItemList *>(&itemlist)));

    QApplication::clipboard()->setText(doc.toString(), QClipboard::Clipboard);

    if (CConfig::inst()->value("/General/Export/OpenBrowser", true).toBool())
        QDesktopServices::openUrl(BrickLink::core()->url(BrickLink::URL_InventoryRequest));
}

void CDocument::fileExportBrickLinkWantedListClipboard(const ItemList &itemlist)
{
    QString wantedlist;

    if (CMessageBox::getString(CFrameWork::inst(), tr("Enter the ID number of Wanted List (leave blank for the default Wanted List)"), wantedlist)) {
        QMap <QString, QString> extra;
        if (!wantedlist.isEmpty())
            extra.insert("WANTEDLISTID", wantedlist);

        QDomDocument doc(QString::null);
        doc.appendChild(BrickLink::core()->createItemListXML(doc, BrickLink::XMLHint_WantedList, reinterpret_cast<const BrickLink::InvItemList *>(&itemlist), extra.isEmpty() ? 0 : &extra));

        QApplication::clipboard()->setText(doc.toString(), QClipboard::Clipboard);

        if (CConfig::inst()->value("/General/Export/OpenBrowser", true).toBool())
            QDesktopServices::openUrl(BrickLink::core()->url(BrickLink::URL_WantedListUpload));
    }
}

void CDocument::fileExportBrickLinkXMLClipboard(const ItemList &itemlist)
{
    QDomDocument doc(QString::null);
    doc.appendChild(BrickLink::core()->createItemListXML(doc, BrickLink::XMLHint_MassUpload, reinterpret_cast<const BrickLink::InvItemList *>(&itemlist)));

    QApplication::clipboard()->setText(doc.toString(), QClipboard::Clipboard);

    if (CConfig::inst()->value("/General/Export/OpenBrowser", true).toBool())
        QDesktopServices::openUrl(BrickLink::core()->url(BrickLink::URL_InventoryUpload));
}

void CDocument::fileExportBrickLinkUpdateClipboard(const ItemList &itemlist)
{
    foreach(const Item *item, itemlist) {
        if (!item->lotId()) {
            if (CMessageBox::warning(CFrameWork::inst(), tr("This list contains items without a BrickLink Lot-ID.<br /><br />Do you really want to export this list?"), CMessageBox::Yes, CMessageBox::No) != CMessageBox::Yes)
                return;
            else
                break;
        }
    }

    QDomDocument doc(QString::null);
    doc.appendChild(BrickLink::core()->createItemListXML(doc, BrickLink::XMLHint_MassUpdate, reinterpret_cast<const BrickLink::InvItemList *>(&itemlist)));

    QApplication::clipboard()->setText(doc.toString(), QClipboard::Clipboard);

    if (CConfig::inst()->value("/General/Export/OpenBrowser", true).toBool())
        QDesktopServices::openUrl(BrickLink::core()->url(BrickLink::URL_InventoryUpdate));
}

void CDocument::fileExportBrickLinkXML(const ItemList &itemlist)
{
    QStringList filters;
    filters << tr("BrickLink XML File") + " (*.xml)";

    QString s = QFileDialog::getSaveFileName(CFrameWork::inst(), tr("Export File"), CConfig::inst()->documentDir(), filters.join(";;"));

    if (!s.isNull()) {
        if (s.right(4) != ".xml")
            s += ".xml";

        if (QFile::exists(s) &&
            CMessageBox::question(CFrameWork::inst(), tr("A file named %1 already exists.Are you sure you want to overwrite it?").arg(CMB_BOLD(s)), CMessageBox::Yes, CMessageBox::No) != CMessageBox::Yes)
            return;

        fileSaveTo(s, "xml", true, itemlist);
    }
}

void CDocument::fileExportBrikTrakInventory(const ItemList &itemlist)
{
    QStringList filters;
    filters << tr("BrikTrak Inventory") + " (*.bti)";

    QString s = QFileDialog::getSaveFileName(CFrameWork::inst(), tr("Export File"), CConfig::inst()->documentDir(), filters.join(";;"));

    if (!s.isNull()) {
        if (s.right(4) != ".bti")
            s += ".bti";

        if (QFile::exists(s) &&
            CMessageBox::question(CFrameWork::inst(), tr("A file named %1 already exists.Are you sure you want to overwrite it?").arg(CMB_BOLD(s)), CMessageBox::Yes, CMessageBox::No) != CMessageBox::Yes)
            return;

        fileSaveTo(s, "bti", true, itemlist);
    }
}

void CDocument::clean2Modified(bool b)
{
    emit modificationChanged(!b);
}

quint64 CDocument::errorMask() const
{
    return m_error_mask;
}

void CDocument::setErrorMask(quint64 em)
{
    m_error_mask = em;
    emit statisticsChanged();
    emit itemsChanged(items(), false);
}

const BrickLink::Order *CDocument::order() const
{
    return m_order;
}

void CDocument::resetDifferences(const ItemList &items)
{
    beginMacro(tr("Reset differences"));

    foreach(Item *pos, items) {
        if ((pos->origQuantity() != pos->quantity()) ||
            (pos->origPrice() != pos->price()))
        {
            Item item = *pos;

            item.setOrigQuantity(item.quantity());
            item.setOrigPrice(item.price());
            changeItem(pos, item);
        }
    }
    endMacro();
}








////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// Itemviews API
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

QModelIndex CDocument::index(int row, int column, const QModelIndex &parent) const
{
    if (hasIndex(row, column, parent))
        return parent.isValid() ? QModelIndex() : createIndex(row, column, m_items.at(row));
    return QModelIndex();
}

CDocument::Item *CDocument::item(const QModelIndex &idx) const
{
    return idx.isValid() ? static_cast<Item *>(idx.internalPointer()) : 0;
}

QModelIndex CDocument::index(const Item *ci) const
{
    Item *i = const_cast<Item *>(ci);

    return i ? createIndex(m_items.indexOf(i), 0, i) : QModelIndex();
}


int CDocument::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : items().size();
}

int CDocument::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : FieldCount;
}

Qt::ItemFlags CDocument::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
                
    Qt::ItemFlags ifs = QAbstractItemModel::flags(index);
    
    switch (index.column()) {
    case Total:
    case LotId: break;
    default   : ifs |= Qt::ItemIsEditable; break;
    }
    return ifs;
}

bool CDocument::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {
        Item *itemp = items().at(index.row());
        Item item = *itemp;
        Field f = static_cast<Field>(index.column());
        
        switch (f) {
        case CDocument::PartNo      : {
/*            const BrickLink::Item *newitem = BrickLink::inst ( )-> item ( m_item-> itemType ( )-> id ( ), result. latin1 ( ));
            if ( newitem ) {
                item. setItem ( newitem );
            }
            else {
                CMessageBox::information ( listView ( ), not_valid );
                return;
            }*/
            break;
        }
        case CDocument::Comments    : item.setComments(value.toString()); break;
        case CDocument::Remarks     : item.setRemarks(value.toString()); break; 
        case CDocument::Reserved    : item.setReserved(value.toString()); break;
        case CDocument::Sale        : item.setSale(value.toInt()); break;
        case CDocument::Bulk        : item.setBulkQuantity(value.toInt()); break;
        case CDocument::TierQ1      : item.setTierQuantity(0, value.toInt()); break;
        case CDocument::TierQ2      : item.setTierQuantity(1, value.toInt()); break;
        case CDocument::TierQ3      : item.setTierQuantity(2, value.toInt()); break;
        case CDocument::TierP1      : item.setTierPrice(0, money_t::fromLocalizedString(value.toString())); break;
        case CDocument::TierP2      : item.setTierPrice(1, money_t::fromLocalizedString(value.toString())); break;
        case CDocument::TierP3      : item.setTierPrice(2, money_t::fromLocalizedString(value.toString())); break;
        case CDocument::Weight      : item.setWeight(CLocaleMeasurement::stringToWeight(value.toString())); break;
        case CDocument::Quantity    : item.setQuantity(value.toInt()); break;
        case CDocument::QuantityDiff: item.setQuantity(itemp->origQuantity() + value.toInt()); break;
        case CDocument::Price       : item.setPrice(money_t::fromLocalizedString(value.toString())); break;
        case CDocument::PriceDiff   : item.setPrice(itemp->origPrice() + money_t::fromLocalizedString(value.toString())); break;
        }
        if (!(item == *itemp)) {
            changeItem(itemp, item);
            emit dataChanged(index, index);
            return true;
        }
    }
    return false;
}
                      
                      
QVariant CDocument::data(const QModelIndex &index, int role) const
{
    //if (role == Qt::DecorationRole)
    //qDebug() << "data() for row " << index.row() << "(picture: " << items().at(index.row())->m_picture << ", item: "<< items().at(index.row())->item()->name();


    if (index.isValid()) {
        Item *it = items().at(index.row());
        Field f = static_cast<Field>(index.column());

        switch (role) {
        case Qt::DisplayRole      : return dataForDisplayRole(it, f);
        case Qt::DecorationRole   : return dataForDecorationRole(it, f);
        case Qt::ToolTipRole      : return dataForToolTipRole(it, f);
        case Qt::TextAlignmentRole: return dataForTextAlignmentRole(it, f);
        }
    }
    return QVariant();
}

QVariant CDocument::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        Field f = static_cast<Field>(section);

        switch (role) {
        case Qt::DisplayRole      : return headerDataForDisplayRole(f);
        case Qt::TextAlignmentRole: return headerDataForTextAlignmentRole(f);
        case Qt::UserRole         : return headerDataForDefaultWidthRole(f);
        }
    }
    return QVariant();
}

QString CDocument::dataForDisplayRole(Item *it, Field f) const
{
    QString dash = QChar('-');

    switch (f) {
    case LotId       : return (it->lotId() == 0 ? dash : QString::number(it->lotId()));
    case PartNo      : return it->item()->id();
    case Description : return it->item()->name();
    case Comments    : return it->comments();
    case Remarks     : return it->remarks();
    case Quantity    : return QString::number(it->quantity());
    case Bulk        : return (it->bulkQuantity() == 1 ? dash : QString::number(it->bulkQuantity()));
    case Price       : return it->price().toLocalizedString();
    case Total       : return it->total().toLocalizedString();
    case Sale        : return (it->sale() == 0 ? dash : QString::number(it->sale()) + "%");
    case Condition   : return (it->condition() == BrickLink::New ? QChar('N') : QChar('U'));
    case Color       : return it->color()->name();
    case Category    : return it->category()->name();
    case ItemType    : return it->itemType()->name();
    case TierQ1      : return (it->tierQuantity(0) == 0 ? dash : QString::number(it->tierQuantity(0)));
    case TierQ2      : return (it->tierQuantity(1) == 0 ? dash : QString::number(it->tierQuantity(1)));
    case TierQ3      : return (it->tierQuantity(2) == 0 ? dash : QString::number(it->tierQuantity(2)));
    case TierP1      : return it->tierPrice(0).toLocalizedString();
    case TierP2      : return it->tierPrice(1).toLocalizedString();
    case TierP3      : return it->tierPrice(2).toLocalizedString();
    case Reserved    : return it->reserved();
    case Weight      : return (it->weight() == 0 ? dash : CLocaleMeasurement().weightToString(it->weight(), true, true));
    case YearReleased: return (it->item()->yearReleased() == 0 ? dash : QString::number(it->item()->yearReleased()));

    case PriceOrig   : return it->origPrice().toLocalizedString();
    case PriceDiff   : return (it->price() - it->origPrice()).toLocalizedString();
    case QuantityOrig: return QString::number(it->origQuantity());
    case QuantityDiff: return QString::number(it->quantity() - it->origQuantity());
    }
    return QString();
}

QVariant CDocument::dataForDecorationRole(Item *it, Field f) const
{
    switch (f) {
    case Picture: {
        return it->image();
    }
    }
    return QPixmap();
}

int CDocument::dataForTextAlignmentRole(Item *, Field f) const
{
    switch (f) {
    case Retain      :
    case Stockroom   :
    case Status      :
    case Picture     :
    case Condition   : return Qt::AlignVCenter | Qt::AlignHCenter;
    case PriceOrig   :
    case PriceDiff   :
    case Price       :
    case Total       :
    case Sale        :
    case TierP1      :
    case TierP2      :
    case TierP3      :
    case Weight      : return Qt::AlignRight | Qt::AlignVCenter;
    default          : return Qt::AlignLeft | Qt::AlignVCenter;
    }
}

QString CDocument::dataForToolTipRole(Item *it, Field f) const
{
    switch (f) {
    case Status: {
        switch (it->status()) {
        case BrickLink::Exclude: return tr("Exclude");
        case BrickLink::Extra  : return tr("Extra");
        case BrickLink::Include: return tr("Include");
        default                : break;
        }
        break;
    }
    case Picture: {
        return dataForDisplayRole(it, PartNo) + " " + dataForDisplayRole(it, Description);
    }
    case Condition: {
        switch (it->condition()) {
        case BrickLink::New : return tr("New");
        case BrickLink::Used: return tr("Used");
        default             : break;
        }
        break;
    }
    case Category: {
        const BrickLink::Category **catpp = it->item()->allCategories();

        if (!catpp[1]) {
            return catpp[0]->name();
        }
        else {
            QString str = QString("<b>%1</b>").arg(catpp [0]->name());
            while (*++catpp)
                str = str + QString("<br />") + catpp [0]->name();
            return str;
        }
        break;
    }
    default: {
        // if (truncated)
        //      return dataForDisplayRole(it, f);
        break;
    }
    }
    return QString();
}


QString CDocument::headerDataForDisplayRole(Field f)
{
    switch (f) {
    case Status      : return tr("Status");
    case Picture     : return tr("Image");
    case PartNo      : return tr("Part #");
    case Description : return tr("Description");
    case Comments    : return tr("Comments");
    case Remarks     : return tr("Remarks");
    case QuantityOrig: return tr("Qty.Orig");
    case QuantityDiff: return tr("Qty.Diff");
    case Quantity    : return tr("Qty.");
    case Bulk        : return tr("Bulk");
    case PriceOrig   : return tr("Pr.Orig");
    case PriceDiff   : return tr("Pr.Diff");
    case Price       : return tr("Price");
    case Total       : return tr("Total");
    case Sale        : return tr("Sale");
    case Condition   : return tr("Cond.");
    case Color       : return tr("Color");
    case Category    : return tr("Category");
    case ItemType    : return tr("Item Type");
    case TierQ1      : return tr("Tier Q1");
    case TierP1      : return tr("Tier P1");
    case TierQ2      : return tr("Tier Q2");
    case TierP2      : return tr("Tier P2");
    case TierQ3      : return tr("Tier Q3");
    case TierP3      : return tr("Tier P3");
    case LotId       : return tr("Lot Id");
    case Retain      : return tr("Retain");
    case Stockroom   : return tr("Stockroom");
    case Reserved    : return tr("Reserved");
    case Weight      : return tr("Weight");
    case YearReleased: return tr("Year");
    }
    return QString();
}

int CDocument::headerDataForTextAlignmentRole(Field f) const
{
    return dataForTextAlignmentRole(0, f);
}

int CDocument::headerDataForDefaultWidthRole(Field f) const
{
    int width = 0;

    switch (f) {
	case Status      : width = -16; break;
	case Picture     : width = -40; break;
	case PartNo      : width = 10; break;
	case Description : width = 28; break;
	case Comments    : width = 8; break;
	case Remarks     : width = 8; break;
	case QuantityOrig: width = 5; break;
	case QuantityDiff: width = 5; break;
	case Quantity    : width = 5; break;
	case Bulk        : width = 5; break;
	case PriceOrig   : width = 8; break;
	case PriceDiff   : width = 8; break;
	case Price       : width = 8; break;
	case Total       : width = 8; break;
	case Sale        : width = 5; break;
	case Condition   : width = 5; break;
	case Color       : width = 15; break;
	case Category    : width = 12; break;
	case ItemType    : width = 12; break;
	case TierQ1      : width = 5; break;
	case TierP1      : width = 8; break;
	case TierQ2      : width = 5; break;
	case TierP2      : width = 8; break;
	case TierQ3      : width = 5; break;
	case TierP3      : width = 8; break;
	case LotId       : width = 8; break;
	case Retain      : width = 8; break;
	case Stockroom   : width = 8; break;
	case Reserved    : width = 8; break;
	case Weight      : width = 10; break;
	case YearReleased: width = 5; break;
    }
    return width;
}


void CDocument::pictureUpdated(BrickLink::Picture *pic)
{
    if (!pic || !pic->item())
        return;

    int row = 0;
    foreach(Item *it, items()) {
        if ((pic->item() == it->item()) && (pic->color() == it->color())) {
            QModelIndex idx = index(row, Picture);
            emit dataChanged(idx, idx);
        }
        row++;
    }
}
