/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
 Qt 5's version of QCache lost the qIsDetached check, that would prevent items being trimmed, when
 they are still in use somewhere else. Although this is an obscure feature and messes with the
 max. cost of the cache (you can easily exceed it this way), it is exactly what BrickStore needs
 to keep tabs on the PriceGuide and Picture objects.
 QCache in Qt 3 and Qt 5 are nearly identical (minus the qIsDetached() check), so this file is a
 copy of the Qt 5 QCache with the check added back in.
*/

template <typename T> inline bool q3IsDetached(T &) { return true; }


#pragma once

#include <QtCore/qhash.h>
#include <QDebug>

QT_BEGIN_NAMESPACE


template <class Key, class T>
class Q3Cache
{
    struct Value
    {
        T *t = nullptr;
        qsizetype cost = 0;
        Value() noexcept = default;
        Value(T *tt, qsizetype c) noexcept
            : t(tt), cost(c)
        {}
        Value(Value &&other) noexcept
            : t(other.t),
              cost(other.cost)
        {
            other.t = nullptr;
        }
        Value &operator=(Value &&other) noexcept
        {
            std::swap(t, other.t);
            std::swap(cost, other.cost);
            return *this;
        }
        ~Value() { delete t; }

    private:
        Q_DISABLE_COPY(Value)
    };

    struct Chain
    {
        Chain() noexcept : prev(this), next(this) { }
        Chain *prev;
        Chain *next;
    };

    struct Node : public Chain
    {
        using KeyType = Key;
        using ValueType = Value;

        Key key;
        Value value;

        Node(const Key &k, Value &&t) noexcept(std::is_nothrow_move_assignable_v<Key>)
            : Chain(),
              key(k),
              value(std::move(t))
        {
        }
        Node(Key &&k, Value &&t) noexcept(std::is_nothrow_move_assignable_v<Key>)
            : Chain(),
              key(std::move(k)),
              value(std::move(t))
        {
        }
        static void createInPlace(Node *n, const Key &k, T *o, qsizetype cost)
        {
            new (n) Node{ Key(k), Value(o, cost) };
        }
        void emplace(T *o, qsizetype cost)
        {
            value = Value(o, cost);
        }

        Node(Node &&other)
            : Chain(other),
              key(std::move(other.key)),
              value(std::move(other.value))
        {
            Q_ASSERT(this->prev);
            Q_ASSERT(this->next);
            this->prev->next = this;
            this->next->prev = this;
        }
    private:
        Q_DISABLE_COPY(Node)
    };

    using Data = QHashPrivate::Data<Node>;

    mutable Chain chain;
    Data d;
    qsizetype mx = 0;
    qsizetype total = 0;

    void unlink(Node *n) noexcept(std::is_nothrow_destructible_v<Node>)
    {
        Q_ASSERT(n->prev);
        Q_ASSERT(n->next);
        n->prev->next = n->next;
        n->next->prev = n->prev;
        total -= n->value.cost;
#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
        auto it = d.find(n->key);
#else
        auto it = d.findBucket(n->key);
#endif
        d.erase(it);
    }
    T *relink(const Key &key) const noexcept
    {
        if (isEmpty())
            return nullptr;
        Node *n = d.findNode(key);
        if (!n)
            return nullptr;

        if (chain.next != n) {
            Q_ASSERT(n->prev);
            Q_ASSERT(n->next);
            n->prev->next = n->next;
            n->next->prev = n->prev;
            n->next = chain.next;
            chain.next->prev = n;
            n->prev = &chain;
            chain.next = n;
        }
        return n->value.t;
    }

    void trim(qsizetype m) noexcept(std::is_nothrow_destructible_v<Node>)
    {
        while (chain.prev != &chain && total > m) {
            Node *n = static_cast<Node *>(chain.prev);
            if (q3IsDetached(*n->value.t))
                unlink(n);
        }
    }

public:
    void clearRecursive()
    {
        auto s = size();
        while (s) {
            trim(0);
            auto new_s = size();
            if (new_s == s)
                break;
            s = new_s;
        }
        if (s)
            qWarning() << "Q3Cache::clearRecursive: clearing" << s << "entries with non-zero ref count";
        clear();
    }

    void setObjectCost(const Key &key, qsizetype cost)
    {
        // Reduce the cost if possible. Increasing is not possible, because the cache might
        // overflow, leaving us in a weird state.
        Node *n = d.findNode(key);
        if (n) {
            qsizetype d = cost - n->value.cost;
            if ((d > 0) && ((total + d) > mx)) {
                //qWarning() << "Q3Cache: adjusting cache object cost by" << d << "would overflow the cache";
            } else if (d != 0) {
                n->value.cost = cost;
                total += d;
                //qWarning() << "Adjusted cache object cost by" << d;
            }
        }
    }

private:
    Q_DISABLE_COPY(Q3Cache)

public:
    inline explicit Q3Cache(qsizetype maxCost = 100) noexcept
        : mx(maxCost)
    {
    }
    inline ~Q3Cache()
    {
        static_assert(std::is_nothrow_destructible_v<Key>, "Types with throwing destructors are not supported in Qt containers.");
        static_assert(std::is_nothrow_destructible_v<T>, "Types with throwing destructors are not supported in Qt containers.");

        clear();
    }

    inline qsizetype maxCost() const noexcept { return mx; }
    void setMaxCost(qsizetype m) noexcept(std::is_nothrow_destructible_v<Node>)
    {
        mx = m;
        trim(mx);
    }
    inline qsizetype totalCost() const noexcept { return total; }

    inline qsizetype size() const noexcept { return qsizetype(d.size); }
    inline qsizetype count() const noexcept { return qsizetype(d.size); }
    inline bool isEmpty() const noexcept { return !d.size; }
    inline QList<Key> keys() const
    {
        QList<Key> k;
        if (size()) {
            k.reserve(size());
            for (auto it = d.begin(); it != d.end(); ++it)
                k << it.node()->key;
        }
        Q_ASSERT(k.size() == size());
        return k;
    }

    void clear() noexcept(std::is_nothrow_destructible_v<Node>)
    {
        d.clear();
        total = 0;
        chain.next = &chain;
        chain.prev = &chain;
    }

    bool insert(const Key &key, T *object, qsizetype cost = 1)
    {
        if (cost > mx) {
            remove(key);
            delete object;
            return false;
        }
        trim(mx - cost);
        auto result = d.findOrInsert(key);
        Node *n = result.it.node();
        if (result.initialized) {
            auto prevCost = n->value.cost;
            result.it.node()->emplace(object, cost);
            cost -= prevCost;
            relink(key);
        } else {
            Node::createInPlace(n, key, object, cost);
            n->prev = &chain;
            n->next = chain.next;
            chain.next->prev = n;
            chain.next = n;
        }
        total += cost;
        return true;
    }
    T *object(const Key &key) const noexcept
    {
        return relink(key);
    }
    T *operator[](const Key &key) const noexcept
    {
        return relink(key);
    }
    inline bool contains(const Key &key) const noexcept
    {
        return !isEmpty() && d.findNode(key) != nullptr;
    }

    bool remove(const Key &key) noexcept(std::is_nothrow_destructible_v<Node>)
    {
        if (isEmpty())
            return false;
        Node *n = d.findNode(key);
        if (!n) {
            return false;
        } else {
            unlink(n);
            return true;
        }
    }

    T *take(const Key &key) noexcept(std::is_nothrow_destructible_v<Key>)
    {
        if (isEmpty())
            return nullptr;
        Node *n = d.findNode(key);
        if (!n)
            return nullptr;

        T *t = n->value.t;
        n->value.t = nullptr;
        unlink(n);
        return t;
    }

};

QT_END_NAMESPACE

