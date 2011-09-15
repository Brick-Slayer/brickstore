/* Copyright (C) 2004-2011 Robert Griebl. All rights reserved.
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
#include <cstdlib>
#include <clocale>
#include <climits>

#include <QApplication>
#include <QLocale>
#include <QValidator>
#include <QRegExp>
#include <QLineEdit>
#include <QKeyEvent>
#include <QDataStream>
#include <QHttp>
#include <QBuffer>
#include <QXmlStreamReader>

#include "application.h"
#include "config.h"
#include "utility.h"
#include "currency.h"
#include "messagebox.h"
#include "framework.h"


// all rates as downloaded are relative to the Euro, but
// we need everything relative to the US Dollar
static void baseConvert(QMap<QString, qreal> &rates)
{
    QMap<QString, qreal>::iterator usd_it = rates.find(QLatin1String("USD"));

    if (usd_it == rates.end()) {
        rates.clear();
        return;
    }
    qreal usd_eur = qreal(1) / usd_it.value();

    for (QMap<QString, qreal>::iterator it = rates.begin(); it != rates.end(); ++it)
        it.value() = (it == usd_it) ? 1 : it.value() *= usd_eur;

    rates.insert(QLatin1String("EUR"), usd_eur);
}


Currency *Currency::s_inst = 0;

Currency::Currency()
    : m_http(0), m_buffer(0)
{
    m_lastUpdate = Config::inst()->value("/Rates/LastUpdate").toDateTime();
    QStringList rates = Config::inst()->value("/Rates/Normal").toString().split(QLatin1Char(','));
    QStringList customRates = Config::inst()->value("/Rates/Custom").toString().split(QLatin1Char(','));

    parseRates(rates, m_rates);
    parseRates(customRates, m_customRates);
}

Currency::~Currency()
{
    Config::inst()->setValue("/Rates/LastUpdate", m_lastUpdate);

    QStringList sl;
    QMap<QString, qreal>::const_iterator it;
    for (it = m_rates.begin(); it != m_rates.end(); ++it) {
        QString s = QLatin1String("%1|%2");
        sl << s.arg(it.key()).arg(it.value());
    }
    Config::inst()->setValue("/Rates/Normal", sl.join(QLatin1String(",")));

    sl.clear();
    for (it = m_customRates.begin(); it != m_customRates.end(); ++it) {
        QString s = QLatin1String("%1|%2");
        sl << s.arg(it.key()).arg(it.value());
    }
    Config::inst()->setValue("/Rates/Custom", sl.join(QLatin1String(",")));

    s_inst = 0;
}

Currency *Currency::inst()
{
    if (!s_inst)
        s_inst = new Currency;
    return s_inst;
}

void Currency::parseRates(const QStringList &ratesList, QMap<QString, double> &ratesMap)
{
    foreach (QString s, ratesList) {
        QStringList sl = s.split(QLatin1Char('|'));
        if (sl.count() == 2) {
            QString sym = sl[0];
            qreal rate = sl[1].toDouble();

            if (!sym.isEmpty() && rate > 0)
                ratesMap.insert(sym, rate);
        }
    }
}


QStringList Currency::currencyCodes() const
{
    return m_rates.keys();
}

QMap<QString, qreal> Currency::rates() const
{
    return m_rates;
}

QMap<QString, qreal> Currency::customRates() const
{
    return m_customRates;
}

qreal Currency::rate(const QString &currencyCode) const
{
    return m_rates.value(currencyCode);
}

qreal Currency::customRate(const QString &currencyCode) const
{
    return m_customRates.value(currencyCode);
}

void Currency::setCustomRate(const QString &currencyCode, qreal rate)
{
    m_customRates.insert(currencyCode, rate);
}

void Currency::unsetCustomRate(const QString &currencyCode)
{
    m_customRates.remove(currencyCode);
}

void Currency::updateRates()
{
    if (m_buffer && m_buffer->isOpen())
        return;

    if (!m_http) {
        m_http = new QHttp(this);
        connect(m_http, SIGNAL(done(bool)), this, SLOT(updateRatesDone(bool)));
        m_buffer = new QBuffer(m_http);
    }
    if (Application::inst()->isOnline()) {
        m_buffer->open(QIODevice::ReadWrite | QIODevice::Truncate);
        m_http->setHost(QLatin1String("www.ecb.europa.eu"));
        m_http->get(QLatin1String("/stats/eurofxref/eurofxref-daily.xml"), m_buffer);
    }
}

void Currency::updateRatesDone(bool error)
{
    m_buffer->close();
    if (error) {
        if (Application::inst()->isOnline())
            MessageBox::warning(FrameWork::inst(), tr("There was an error downloading the exchange rates from the ECB server:<br>%2").arg(m_http->errorString()));
    } else {
        QXmlStreamReader reader(m_buffer->data());
        QMap<QString, qreal> newRates;
        QLocale c = QLocale::c();

        while (!reader.atEnd()) {
            if (reader.readNext() == QXmlStreamReader::StartElement &&
                reader.name() == QLatin1String("Cube") &&
                reader.attributes().hasAttribute(QLatin1String("currency"))) {

                QString currency = reader.attributes().value(QLatin1String("currency")).toString();
                qreal rate = c.toDouble(reader.attributes().value(QLatin1String("rate")).toString());

                if (currency.length() == 3 && rate > 0)
                    newRates.insert(currency, rate);
            }
        }
        baseConvert(newRates);

        if (reader.hasError() || newRates.isEmpty()) {
            QString err;
            if (reader.hasError())
                err = tr("%1 (line %2, column %3)").arg(reader.errorString()).arg(reader.lineNumber()).arg(reader.columnNumber());
            else
                err = tr("no currency data found");
            MessageBox::warning(FrameWork::inst(), tr("There was an error parsing the exchange rates from the ECB server:\n%1").arg(err));
            qWarning() << m_buffer->data();
        } else {
            m_rates = newRates;
            m_lastUpdate = QDateTime::currentDateTime();
            emit ratesChanged();
        }
    }
    m_buffer->buffer().clear();
}

QDateTime Currency::lastUpdate() const
{
    return m_lastUpdate;
}

QString Currency::toString(double value, const QString &currencyCode, Symbol cs, int precision)
{
    QLocale loc;
    loc.setNumberOptions(QLocale::OmitGroupSeparator);
    QString s = loc.toString(value, 'f', precision);

    if (cs == LocalSymbol)
        return localSymbol(currencyCode) + QLatin1Char(' ') + s;
    else if (cs == InternationalSymbol)
        return currencyCode + QLatin1Char(' ') + s;
    else
        return s;
}

double Currency::fromString(const QString &str)
{
    QString s = str.trimmed();

    if (s.isEmpty())
        return 0;
    return QLocale().toDouble(s);
}

QString Currency::localSymbol(const QString &intSymbol)
{
    return Utility::localForInternationalCurrencySymbol(intSymbol);
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

class DotCommaFilter : public QObject {
public:
    DotCommaFilter(QObject *parent)
        : QObject(parent)
    { }

protected:
    bool eventFilter(QObject *o, QEvent *e)
    {
        if (((e->type() == QEvent::KeyPress) || (e->type() == QEvent::KeyRelease)) && qobject_cast<QLineEdit *>(o)) {
            const CurrencyValidator *val = qobject_cast<const CurrencyValidator *>(static_cast<QLineEdit *>(o)->validator());

            if (val) {
                QKeyEvent *ke = static_cast<QKeyEvent *>(e);

                QString text = ke->text();
                bool fixed = false;
                QLocale loc;

                for (int i = 0; i < text.length(); ++i) {
                    QCharRef ir = text[i];
                    if (ir == QLatin1Char('.') || ir == QLatin1Char(',')) {
                        ir = loc.decimalPoint();
                        fixed = true;
                    }
                }

                if (fixed)
                    *ke = QKeyEvent(ke->type(), ke->key(), ke->modifiers(), text, ke->isAutoRepeat(), ke->count());
            }
        }
        return false;
    }
};

bool CurrencyValidator::s_once = false;

CurrencyValidator::CurrencyValidator(QObject *parent)
    : QDoubleValidator(parent)
{
    if (!s_once) {
        s_once = true;
        qApp->installEventFilter(new DotCommaFilter(qApp));
    }
}

CurrencyValidator::CurrencyValidator(double bottom, double top, int decimals, QObject *parent)
    : QDoubleValidator(bottom, top, decimals, parent)
{
    if (!s_once) {
        s_once = true;
        qApp->installEventFilter(new DotCommaFilter(qApp));
    }
}

QValidator::State CurrencyValidator::validate(QString &input, int &pos) const
{
    double b = bottom(), t = top();
    int d = decimals();

//    double f = Currency::rate();
//    b *= f;
//    t *= f;

    QChar dp = QLocale().decimalPoint();

    QRegExp r(QString(QLatin1String(" *-?\\d*\\%1?\\d* *")).arg(dp));

    if (b >= 0 && input.simplified().startsWith(QLatin1Char('-')))
        return Invalid;

    if (r.exactMatch(input)) {
        QString s = input;
        s.replace(dp, QLatin1Char('.'));

        int i = s.indexOf(QLatin1Char('.'));
        if (i >= 0) {
            // has decimal point, now count digits after that
            i++;
            int j = i;
            while (s [j].isDigit())
                j++;
            if (j > i) {
                while (s [j - 1] == QLatin1Char('0'))
                    j--;
            }

            if (j - i > d) {
                return Intermediate;
            }
        }

        double entered = s.toDouble();

        if (entered < b || entered > t)
            return Intermediate;
        else
            return Acceptable;
    }
    else if (r.matchedLength() == (int) input.length()) {
        return Intermediate;
    }
    else {
        pos = input.length();
        return Invalid;
    }
}

