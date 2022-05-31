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
#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QObject>

#include "core.h"
#include "color.h"
#include "itemtype.h"
#include "category.h"
#include "item.h"
#include "lot.h"
#include "store.h"
#include "order.h"
#include "cart.h"


class QmlDocumentLots;

namespace BrickLink {

class QmlColor;
class QmlItemType;
class QmlCategory;
class QmlItem;
class QmlLot;

class QmlBrickLink : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    Q_PRIVATE_PROPERTY(d, QString cachePath READ dataPath CONSTANT)
    Q_PRIVATE_PROPERTY(d, QSize standardPictureSize READ standardPictureSize CONSTANT)

    Q_PROPERTY(QmlItem noItem READ noItem CONSTANT)
    Q_PROPERTY(QmlColor noColor READ noColor CONSTANT)
    Q_PRIVATE_PROPERTY(d, BrickLink::Store *store READ store CONSTANT)
    Q_PRIVATE_PROPERTY(d, BrickLink::Orders *orders READ orders CONSTANT)
    Q_PRIVATE_PROPERTY(d, BrickLink::Carts *carts READ carts CONSTANT)
    Q_PRIVATE_PROPERTY(d, BrickLink::Database *database READ database CONSTANT)

public:
    static void registerTypes();

    QmlBrickLink(Core *core);

    // copied from namespace BrickLink
    enum class Time          { PastSix, Current };
    enum class Price         { Lowest, Average, WAverage, Highest };
    enum class Condition     { New, Used };
    enum class SubCondition  { None, Complete, Incomplete, Sealed };
    enum class Stockroom     { None, A, B, C };
    enum class Status        { Include, Exclude, Extra };
    enum class UpdateStatus  { Ok, Loading, Updating, UpdateFailed };

    enum class OrderType     { Received, Placed, Any };
    enum class OrderStatus   { Unknown, Pending, Updated, Processing, Ready, Paid, Packed, Shipped,
                               Received, Completed, OCR, NPB, NPX, NRS, NSS, Cancelled, Count };

    Q_ENUM(Time)
    Q_ENUM(Price)
    Q_ENUM(Condition)
    Q_ENUM(SubCondition)
    Q_ENUM(Stockroom)
    Q_ENUM(Status)
    Q_ENUM(UpdateStatus)
    Q_ENUM(OrderType)
    Q_ENUM(OrderStatus)

    QmlItem noItem() const;
    QmlColor noColor() const;

    Q_INVOKABLE QImage noImage(int width, int height) const;

    Q_INVOKABLE BrickLink::QmlColor color(const QVariant &v) const;
    Q_INVOKABLE BrickLink::QmlColor colorFromLDrawId(int ldrawId) const;
    Q_INVOKABLE BrickLink::QmlCategory category(const QVariant &v) const;
    Q_INVOKABLE BrickLink::QmlItemType itemType(const QVariant &v) const;
    Q_INVOKABLE BrickLink::QmlItem item(const QVariant &v) const;
    Q_INVOKABLE BrickLink::QmlItem item(const QString &itemTypeId, const QString &itemId) const;

    Q_INVOKABLE BrickLink::PriceGuide *priceGuide(BrickLink::QmlItem item, BrickLink::QmlColor color,
                                                  bool highPriority = false);

    Q_INVOKABLE BrickLink::Picture *picture(BrickLink::QmlItem item, BrickLink::QmlColor color,
                                            bool highPriority = false);
    Q_INVOKABLE BrickLink::Picture *largePicture(BrickLink::QmlItem item, bool highPriority = false);

    Q_INVOKABLE BrickLink::QmlLot lot(const QVariant &v) const;

    Q_INVOKABLE void cacheStat() const;

    Q_INVOKABLE QString itemHtmlDescription(BrickLink::QmlItem item, BrickLink::QmlColor color,
                                            const QColor &highlight) const;

signals:
    void priceGuideUpdated(BrickLink::PriceGuide *priceGuide);
    void pictureUpdated(BrickLink::Picture *picture);

private:
    static char firstCharInString(const QString &str);

    static QmlBrickLink *s_inst;
    Core *d;
};


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


template <typename T> class QmlWrapperBase
{
public:
    inline T *wrappedObject() const
    {
        return (wrapped == wrappedNull()) ? nullptr : wrapped;
    }
    inline bool isNull() const { return !wrappedObject(); }

protected:
    QmlWrapperBase(T *_wrappedObject)
        : wrapped(_wrappedObject ? _wrappedObject : wrappedNull())
    { }
    QmlWrapperBase(const QmlWrapperBase &copy)
        : wrapped(copy.wrapped)
    { }
    virtual ~QmlWrapperBase() = default;

    static T *wrappedNull()
    {
        static T t_null(nullptr);
        return &t_null;
    }
    T *wrapped;
};


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


class QmlColor : public QmlWrapperBase<const Color>
{
    Q_GADGET
    Q_PROPERTY(bool isNull READ isNull)

    Q_PRIVATE_PROPERTY(wrapped, int id READ id CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString name READ name CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QColor color READ color CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, int ldrawId READ ldrawId CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QColor ldrawColor READ ldrawColor CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QColor ldrawEdgeColor READ ldrawEdgeColor CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool solid READ isSolid CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool transparent READ isTransparent CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool glitter READ isGlitter CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool speckle READ isSpeckle CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool metallic READ isMetallic CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool chrome READ isChrome CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool pearl READ isPearl CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool milky READ isMilky CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool modulex READ isModulex CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool satin READ isSatin CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double popularity READ popularity CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, float luminance READ luminance CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool particles READ hasParticles CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, float particleMinSize READ particleMinSize CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, float particleMaxSize READ particleMaxSize CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, float particleFraction READ particleFraction CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, float particleVFraction READ particleVFraction CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QColor particleColor READ particleColor CONSTANT)

public:
    QmlColor(const Color *col = nullptr);

    Q_INVOKABLE QImage image(int width, int height) const;

    friend class QmlBrickLink;
    friend class QmlLot;
};


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


class QmlItemType : public QmlWrapperBase<const ItemType>
{
    Q_GADGET
    Q_PROPERTY(bool isNull READ isNull)

    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString name READ name CONSTANT)
    Q_PROPERTY(QVariantList categories READ categories CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool hasInventories READ hasInventories CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool hasColors READ hasColors CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool hasWeight READ hasWeight CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool hasSubConditions READ hasSubConditions CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QSize pictureSize READ pictureSize CONSTANT)

public:
    QmlItemType(const ItemType *itt = nullptr);

    QString id() const;
    QVariantList categories() const;

    friend class QmlBrickLink;
};


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


class QmlCategory : public QmlWrapperBase<const Category>
{
    Q_GADGET
    Q_PROPERTY(bool isNull READ isNull)

    Q_PRIVATE_PROPERTY(wrapped, int id READ id CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString name READ name CONSTANT)

public:
    QmlCategory(const Category *cat = nullptr);

    friend class QmlBrickLink;
};


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


class QmlItem : public QmlWrapperBase<const Item>
{
    Q_GADGET
    Q_PROPERTY(bool isNull READ isNull)

    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QString name READ name CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QmlItemType itemType READ itemType CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QmlCategory category READ category CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, bool hasInventory READ hasInventory CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QDateTime inventoryUpdated READ inventoryUpdated CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, QmlColor defaultColor READ defaultColor CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, double weight READ weight CONSTANT)
    Q_PRIVATE_PROPERTY(wrapped, int yearReleased READ yearReleased CONSTANT)
    Q_PROPERTY(QVariantList knownColors READ knownColors CONSTANT)

public:
    QmlItem(const Item *item = nullptr);

    QString id() const;
    Q_INVOKABLE bool hasKnownColor(BrickLink::QmlColor color) const;
    QVariantList knownColors() const;

    Q_INVOKABLE QVariantList consistsOf() const;

    // tough .. BrickLink::AppearsIn appearsIn(const Color *color = nullptr) const;

    friend class QmlBrickLink;
};


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


class QmlBrickLink;

class QmlLot : public QmlWrapperBase<Lot>
{
    Q_GADGET
    Q_PROPERTY(bool isNull READ isNull)

    Q_PROPERTY(QmlItem item READ item WRITE setItem)
    Q_PROPERTY(QmlColor color READ color WRITE setColor)
    Q_PROPERTY(QmlCategory category READ category)
    Q_PROPERTY(QmlItemType itemType READ itemType)
    Q_PROPERTY(QString itemId READ itemId)
    Q_PROPERTY(QString id READ itemId)
    Q_PROPERTY(QString itemName READ itemName)
    Q_PROPERTY(QString name READ itemName)
    Q_PROPERTY(QString colorName READ colorName)
    Q_PROPERTY(QString categoryName READ categoryName)
    Q_PROPERTY(QString itemTypeName READ itemTypeName)
    Q_PROPERTY(int itemYearReleased READ itemYearReleased)

    Q_PROPERTY(QmlBrickLink::Status status READ status WRITE setStatus)
    Q_PROPERTY(QmlBrickLink::Condition condition READ condition WRITE setCondition)
    Q_PROPERTY(QmlBrickLink::SubCondition subCondition READ subCondition WRITE setSubCondition)

    Q_PROPERTY(QString comments READ comments WRITE setComments)
    Q_PROPERTY(QString remarks READ remarks WRITE setRemarks)

    Q_PROPERTY(int quantity READ quantity WRITE setQuantity)
    Q_PROPERTY(int bulkQuantity READ bulkQuantity WRITE setBulkQuantity)
    Q_PROPERTY(int tier1Quantity READ tier1Quantity WRITE setTier1Quantity)
    Q_PROPERTY(int tier2Quantity READ tier2Quantity WRITE setTier2Quantity)
    Q_PROPERTY(int tier3Quantity READ tier3Quantity WRITE setTier3Quantity)

    Q_PROPERTY(double price READ price WRITE setPrice)
    Q_PROPERTY(double tier1Price READ tier1Price WRITE setTier1Price)
    Q_PROPERTY(double tier2Price READ tier2Price WRITE setTier2Price)
    Q_PROPERTY(double tier3Price READ tier3Price WRITE setTier3Price)

    Q_PROPERTY(int sale READ sale WRITE setSale)
    Q_PROPERTY(double total READ total)

    Q_PROPERTY(uint lotId READ lotId WRITE setLotId)
    Q_PROPERTY(bool retain READ retain WRITE setRetain)
    Q_PROPERTY(QmlBrickLink::Stockroom stockroom READ stockroom WRITE setStockroom)

    Q_PROPERTY(double totalWeight READ totalWeight WRITE setTotalWeight)
    Q_PROPERTY(QString reserved READ reserved WRITE setReserved)
    Q_PROPERTY(bool alternate READ alternate WRITE setAlternate)
    Q_PROPERTY(uint alternateId READ alternateId WRITE setAlternateId)
    Q_PROPERTY(bool counterPart READ counterPart WRITE setCounterPart)

    Q_PROPERTY(bool incomplete READ incomplete)

    Q_PROPERTY(QImage image READ image)

public:
    QmlLot(Lot *lot = nullptr, ::QmlDocumentLots *documentLots = nullptr);
    QmlLot(const QmlLot &copy);
    QmlLot(QmlLot &&move);
    ~QmlLot() override;

    static QmlLot create(Lot * &&lot); // QmlLot owns the lot
    QmlLot &operator=(const QmlLot &assign);

    QmlItem item() const               { return get()->item(); }
    void setItem(QmlItem item)         { set().to()->setItem(item.wrappedObject()); }
    QmlColor color() const             { return get()->color(); }
    void setColor(QmlColor color)      { set().to()->setColor(color.wrappedObject()); }
    QmlCategory category() const       { return get()->category(); }
    QmlItemType itemType() const       { return get()->itemType(); }

    QString itemId() const             { return QString::fromLatin1(get()->itemId()); }
    QString itemName() const           { return get()->itemName(); }
    QString colorName() const          { return get()->colorName(); }
    QString categoryName() const       { return get()->categoryName(); }
    QString itemTypeName() const       { return get()->itemTypeName(); }
    int itemYearReleased() const       { return get()->itemYearReleased(); }

    QmlBrickLink::Status status() const                { return static_cast<QmlBrickLink::Status>(get()->status()); }
    void setStatus(QmlBrickLink::Status s)             { set().to()->setStatus(static_cast<Status>(s)); }
    QmlBrickLink::Condition condition() const          { return static_cast<QmlBrickLink::Condition>(get()->condition()); }
    void setCondition(QmlBrickLink::Condition c)       { set().to()->setCondition(static_cast<Condition>(c)); }
    QmlBrickLink::SubCondition subCondition() const    { return static_cast<QmlBrickLink::SubCondition>(get()->subCondition()); }
    void setSubCondition(QmlBrickLink::SubCondition c) { set().to()->setSubCondition(static_cast<SubCondition>(c)); }
    QString comments() const           { return get()->comments(); }
    void setComments(const QString &n) { set().to()->setComments(n); }
    QString remarks() const            { return get()->remarks(); }
    void setRemarks(const QString &r)  { set().to()->setRemarks(r); }

    int quantity() const               { return get()->quantity(); }
    void setQuantity(int q)            { set().to()->setQuantity(q); }
    int bulkQuantity() const           { return get()->bulkQuantity(); }
    void setBulkQuantity(int q)        { set().to()->setBulkQuantity(q); }
    int tier1Quantity() const          { return get()->tierQuantity(0); }
    void setTier1Quantity(int q)       { set().to()->setTierQuantity(0, q); }
    int tier2Quantity() const          { return get()->tierQuantity(1); }
    void setTier2Quantity(int q)       { set().to()->setTierQuantity(1, q); }
    int tier3Quantity() const          { return get()->tierQuantity(2); }
    void setTier3Quantity(int q)       { set().to()->setTierQuantity(2, q); }

    double price() const               { return get()->price(); }
    void setPrice(double p)            { set().to()->setPrice(p); }
    double tier1Price() const          { return get()->tierPrice(0); }
    void setTier1Price(double p)       { set().to()->setTierPrice(0, p); }
    double tier2Price() const          { return get()->tierPrice(1); }
    void setTier2Price(double p)       { set().to()->setTierPrice(1, p); }
    double tier3Price() const          { return get()->tierPrice(2); }
    void setTier3Price(double p)       { set().to()->setTierPrice(2, p); }

    int sale() const                   { return get()->sale(); }
    void setSale(int s)                { set().to()->setSale(s); }
    double total() const               { return get()->total(); }

    uint lotId() const                 { return get()->lotId(); }
    void setLotId(uint lid)            { set().to()->setLotId(lid); }

    bool retain() const                { return get()->retain(); }
    void setRetain(bool r)             { set().to()->setRetain(r); }
    QmlBrickLink::Stockroom stockroom() const     { return static_cast<QmlBrickLink::Stockroom>(get()->stockroom()); }
    void setStockroom(QmlBrickLink::Stockroom sr) { set().to()->setStockroom(static_cast<Stockroom>(sr)); }

    double totalWeight() const         { return get()->totalWeight(); }
    void setTotalWeight(double w)      { set().to()->setTotalWeight(w); }

    QString reserved() const           { return get()->reserved(); }
    void setReserved(const QString &r) { set().to()->setReserved(r); }

    bool alternate() const             { return get()->alternate(); }
    void setAlternate(bool a)          { set().to()->setAlternate(a); }
    uint alternateId() const           { return get()->alternateId(); }
    void setAlternateId(uint aid)      { set().to()->setAlternateId(aid); }

    bool counterPart() const           { return get()->counterPart(); }
    void setCounterPart(bool b)        { set().to()->setCounterPart(b); }

    bool incomplete() const            { return get()->isIncomplete(); }

    QImage image() const;

private:
    class Setter
    {
    public:
        Setter(QmlLot *lot);
        Setter(const Setter &copy) = default;
        Lot *to();
        ~Setter();

    private:
        // the definition is in document.cpp
        void doChangeLot(::QmlDocumentLots *lots, Lot *which, const Lot &value);
        QmlLot *m_lot;
        Lot m_to;
    };
    Setter set();
    Lot *get() const;

    QmlDocumentLots *m_documentLots = nullptr;
    constexpr static quintptr Owning = 1u;

    friend class Setter;
    friend class ::QmlDocumentLots;
};

} // namespace BrickLink

Q_DECLARE_METATYPE(BrickLink::QmlColor)
Q_DECLARE_METATYPE(BrickLink::QmlItemType)
Q_DECLARE_METATYPE(BrickLink::QmlCategory)
Q_DECLARE_METATYPE(BrickLink::QmlItem)
Q_DECLARE_METATYPE(BrickLink::QmlLot)
Q_DECLARE_METATYPE(BrickLink::QmlBrickLink::Time)
Q_DECLARE_METATYPE(BrickLink::QmlBrickLink::Price)
Q_DECLARE_METATYPE(BrickLink::QmlBrickLink::Condition)
Q_DECLARE_METATYPE(BrickLink::QmlBrickLink::Stockroom)
Q_DECLARE_METATYPE(BrickLink::QmlBrickLink::Status)
Q_DECLARE_METATYPE(BrickLink::QmlBrickLink::UpdateStatus)
Q_DECLARE_METATYPE(BrickLink::QmlBrickLink::OrderType)
Q_DECLARE_METATYPE(BrickLink::QmlBrickLink *)