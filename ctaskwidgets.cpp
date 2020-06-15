/* Copyright (C) 2013-2014 Patrick Brans.  All rights reserved.
**
** This file is part of BrickStock.
** BrickStock is based heavily on BrickStore (http://www.brickforge.de/software/brickstore/)
** by Robert Griebl, Copyright (C) 2004-2008.
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

#include <QDockWidget>
#include <qlabel.h>
//Added by qt3to4:
#include <QFrame>
#include <QPixmap>
#include <QEvent>

#include "bricklink.h"
#include "cconfig.h"
#include "cresource.h"
#include "cutility.h"
#include "curllabel.h"
#include "cframework.h"

#include "ctaskwidgets.h"


CTaskLinksWidget::CTaskLinksWidget ( QWidget *parent, const char *name )
	: CUrlLabel ( parent, name ), m_doc ( 0 )
{
    setFrameStyle ( QFrame::StyledPanel | QFrame::Sunken );

	connect ( CFrameWork::inst ( ), SIGNAL( documentActivated ( CDocument * )), this, SLOT( documentUpdate ( CDocument * )));

	unsetPalette ( );
    setText ( "<b>ABCDEFGHIJKLM</b><br />1<br />2<br />3<br />4<br /><br /><b>X</b><br />1<br />" );
	setMinimumSize ( sizeHint ( ));
    setText ( QString ( ));
}

void CTaskLinksWidget::documentUpdate ( CDocument *doc )
{
	if ( m_doc )
		disconnect ( m_doc, SIGNAL( selectionChanged ( const CDocument::ItemList & )), this, SLOT( selectionUpdate ( const CDocument::ItemList & )));
	m_doc = doc;
	if ( m_doc )
		connect ( m_doc, SIGNAL( selectionChanged ( const CDocument::ItemList & )), this, SLOT( selectionUpdate ( const CDocument::ItemList & )));

	selectionUpdate ( m_doc ? m_doc-> selection ( ) : CDocument::ItemList ( ));
}

void CTaskLinksWidget::selectionUpdate ( const CDocument::ItemList &list )
{
	QString str;

	if ( m_doc && ( list. count ( ) == 1 )) {
		const BrickLink::Item *item   = list. front ( )-> item ( );
		const BrickLink::Color *color = list. front ( )-> color ( );

		if ( item && color ) {
			QString fmt1 = "&nbsp;&nbsp;<b>%1</b><br />";
			QString fmt2 = "&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"%2\">%1...</a><br />";

			str += fmt1. arg( tr( "BrickLink" ));
			if ( list. front ( )-> lotId ( )) {
				uint lotid = list. front ( )-> lotId ( );
				str += fmt2. arg( tr( "My Inventory" )). arg( BrickLink::inst ( )-> url ( BrickLink::URL_StoreItemDetail, &lotid ));
			}

			str += fmt2. arg( tr( "Catalog" )).         arg( BrickLink::inst ( )-> url ( BrickLink::URL_CatalogInfo,    item, color ));
			str += fmt2. arg( tr( "Price Guide" )).     arg( BrickLink::inst ( )-> url ( BrickLink::URL_PriceGuideInfo, item, color ));
			str += fmt2. arg( tr( "Lots for Sale" )).   arg( BrickLink::inst ( )-> url ( BrickLink::URL_LotsForSale,    item, color ));

			str += "<br />";

			str += fmt1. arg( tr( "Peeron" ));
			str += fmt2. arg( tr( "Information" )). arg( BrickLink::inst ( )-> url ( BrickLink::URL_PeeronInfo, item, color ));
		}
	}
    setText ( str );
}

void CTaskLinksWidget::languageChange ( )
{
	if ( m_doc )
		selectionUpdate ( m_doc-> selection ( ));
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

CTaskPriceGuideWidget::CTaskPriceGuideWidget ( QWidget *parent, const char *name )
	: CPriceGuideWidget ( parent, name ), m_doc ( 0 )
{
    setFrameStyle ( QFrame::StyledPanel | QFrame::Sunken );

	connect ( CFrameWork::inst ( ), SIGNAL( documentActivated ( CDocument * )), this, SLOT( documentUpdate ( CDocument * )));
	connect ( this, SIGNAL( priceDoubleClicked ( money_t )), this, SLOT( setPrice ( money_t )));
	fixParentDockWindow ( );
}

void CTaskPriceGuideWidget::documentUpdate ( CDocument *doc )
{
	if ( m_doc )
		disconnect ( m_doc, SIGNAL( selectionChanged ( const CDocument::ItemList & )), this, SLOT( selectionUpdate ( const CDocument::ItemList & )));
	m_doc = doc;
	if ( m_doc )
		connect ( m_doc, SIGNAL( selectionChanged ( const CDocument::ItemList & )), this, SLOT( selectionUpdate ( const CDocument::ItemList & )));

	selectionUpdate ( m_doc ? m_doc-> selection ( ) : CDocument::ItemList ( ));
}

void CTaskPriceGuideWidget::selectionUpdate ( const CDocument::ItemList &list )
{
	bool ok = ( m_doc && ( list. count ( ) == 1 ));

	setPriceGuide ( ok ? BrickLink::inst ( )-> priceGuide ( list. front ( )-> item ( ), list. front ( )-> color ( ), true ) : 0 );
}

void CTaskPriceGuideWidget::setPrice ( money_t p )
{
	if ( m_doc && ( m_doc-> selection ( ). count ( ) == 1 )) {
		CDocument::Item *pos = m_doc-> selection ( ). front ( );
		CDocument::Item item = *pos;

		item. setPrice ( p );
		m_doc-> changeItem ( pos, item );
	}
}

bool CTaskPriceGuideWidget::event ( QEvent *e )
{
    if ( e-> type ( ) == QEvent::ParentChange )
		fixParentDockWindow ( );

	return CPriceGuideWidget::event ( e );
}

void CTaskPriceGuideWidget::fixParentDockWindow ( )
{
	disconnect ( this, SLOT( setOrientation ( Qt::Orientation )));

	for ( QObject *p = parent( ); p; p = p-> parent ( )) {
        if ( p-> inherits ( "QDockWidget" )) {
            //connect ( p, SIGNAL( orientationChanged ( Qt::Orientation )), this, SLOT( setOrientation ( Qt::Orientation )));
            if (static_cast <QDockWidget *> ( p )->features() & QDockWidget::DockWidgetVerticalTitleBar) {
                setOrientation ( Qt::Horizontal);
            } else {
                 setOrientation ( Qt::Vertical);
            }
			break;
		}
	}
}

void CTaskPriceGuideWidget::setOrientation ( Qt::Orientation o )
{
	setLayout ( o == Qt::Horizontal ? CPriceGuideWidget::Horizontal : CPriceGuideWidget::Vertical );
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

CTaskInfoWidget::CTaskInfoWidget ( QWidget *parent, const char * )
    : QStackedWidget ( parent ), m_doc ( 0 )
{
    setFrameStyle ( QFrame::StyledPanel | QFrame::Sunken );
    m_pic = new CPictureWidget ( this );
	m_text = new QLabel ( this );
	m_text-> setAlignment ( Qt::AlignLeft | Qt::AlignTop );
	m_text-> setIndent ( 8 );

    addWidget ( m_pic );
    addWidget ( m_text );

    paletteChange ( palette ( ));
    setBackgroundColor(Qt::white);

	connect ( CFrameWork::inst ( ), SIGNAL( documentActivated ( CDocument * )), this, SLOT( documentUpdate ( CDocument * )));
	connect ( CMoney::inst ( ), SIGNAL( monetarySettingsChanged ( )), this, SLOT( refresh ( )));
	connect ( CConfig::inst ( ), SIGNAL( weightSystemChanged ( CConfig::WeightSystem )), this, SLOT( refresh ( )));
}

void CTaskInfoWidget::documentUpdate ( CDocument *doc )
{
	if ( m_doc )
		disconnect ( m_doc, SIGNAL( selectionChanged ( const CDocument::ItemList & )), this, SLOT( selectionUpdate ( const CDocument::ItemList & )));
	m_doc = doc;
	if ( m_doc )
		connect ( m_doc, SIGNAL( selectionChanged ( const CDocument::ItemList & )), this, SLOT( selectionUpdate ( const CDocument::ItemList & )));

	selectionUpdate ( m_doc ? m_doc-> selection ( ) : CDocument::ItemList ( ));
}

void CTaskInfoWidget::selectionUpdate ( const CDocument::ItemList &list )
{
	if ( !m_doc || ( list. count ( ) == 0 )) {
		m_pic-> setPicture ( 0 );
        setCurrentIndex(0);
	}
	else if ( list. count ( ) == 1 ) {
		m_pic-> setPicture ( BrickLink::inst ( )-> picture ( list. front ( )-> item ( ), list. front ( )-> color ( ), true ));
        setCurrentIndex(0);
	}
	else {
		CDocument::Statistics stat = m_doc-> statistics ( list );

		QString s;
		QString valstr, wgtstr;

		if ( stat. value ( ) != stat. minValue ( )) {
			valstr = QString ( "%1 (%2 %3)" ).
						arg( stat. value ( ). toLocalizedString ( true )).
						arg( tr( "min." )).
						arg( stat. minValue ( ). toLocalizedString ( true ));
		}
		else
			valstr = stat. value ( ). toLocalizedString ( true );

		if ( stat. weight ( ) == -DBL_MIN ) {
			wgtstr = "-";
		}
		else {
			double weight = stat. weight ( );

			if ( weight < 0 ) {
				weight = -weight;
				wgtstr = tr( "min." ) + " ";
			}

			wgtstr += CUtility::weightToString ( weight, ( CConfig::inst ( )-> weightSystem ( ) == CConfig::WeightImperial ), true, true );
		}

		s = QString ( "<h3>%1</h3>&nbsp;&nbsp;%2: %3<br />&nbsp;&nbsp;%4: %5<br /><br />&nbsp;&nbsp;%6: %7<br /><br />&nbsp;&nbsp;%8: %9" ).
			arg ( tr( "Multiple lots selected" )).
			arg ( tr( "Lots" )). arg ( stat. lots ( )).
			arg ( tr( "Items" )). arg ( stat. items ( )).
			arg ( tr( "Value" )). arg ( valstr ).
			arg ( tr( "Weight" )). arg ( wgtstr );

//		if (( stat. errors ( ) > 0 ) && CConfig::inst ( )-> showInputErrors ( ))
//			s += QString ( "<br /><br />&nbsp;&nbsp;%1: %2" ). arg ( tr( "Errors" )). arg ( stat. errors ( ));

		m_pic-> setPicture ( 0 );
		m_text-> setText ( s );
        setCurrentIndex(1);
	}
}

void CTaskInfoWidget::languageChange ( )
{
	refresh ( );
}

void CTaskInfoWidget::refresh ( )
{
	if ( m_doc )
		selectionUpdate ( m_doc-> selection ( ));
}

void CTaskInfoWidget::paletteChange ( const QPalette &oldpal )
{
    QPalette palette = m_text->palette();
    palette.setBrush(m_text->backgroundRole(), QBrush(CResource::inst ( )-> pixmap( "bg_infotext" )));
    m_text->setPalette(palette);
    m_text->setAutoFillBackground(true);

    palette = this->palette();
    palette.setColor(this->backgroundRole(), Qt::white);
    this->setPalette(palette);
    this->setAutoFillBackground(true);

    QStackedWidget::paletteChange ( oldpal );
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

CTaskAppearsInWidget::CTaskAppearsInWidget ( QWidget *parent, const char *name )
	: CAppearsInWidget ( parent, name ), m_doc ( 0 )
{
	connect ( CFrameWork::inst ( ), SIGNAL( documentActivated ( CDocument * )), this, SLOT( documentUpdate ( CDocument * )));
}

QSize CTaskAppearsInWidget::minimumSizeHint ( ) const
{
	const QFontMetrics &fm = fontMetrics ( );

	return QSize ( fm. width ( 'm' ) * 20, fm.height ( ) * 10 );
}

QSize CTaskAppearsInWidget::sizeHint ( ) const
{
	return minimumSizeHint ( );
}

void CTaskAppearsInWidget::documentUpdate ( CDocument *doc )
{
	if ( m_doc )
		disconnect ( m_doc, SIGNAL( selectionChanged ( const CDocument::ItemList & )), this, SLOT( selectionUpdate ( const CDocument::ItemList & )));
	m_doc = doc;
	if ( m_doc )
		connect ( m_doc, SIGNAL( selectionChanged ( const CDocument::ItemList & )), this, SLOT( selectionUpdate ( const CDocument::ItemList & )));

	selectionUpdate ( m_doc ? m_doc-> selection ( ) : CDocument::ItemList ( ));
}

void CTaskAppearsInWidget::selectionUpdate ( const CDocument::ItemList &list )
{
    if ( !m_doc || list. isEmpty ( ))
        setItem ( 0, 0 );
    else if ( list. count ( ) == 1 )
    	setItem ((*list. front ( )). item ( ), (*list. front ( )). color ( ));
    else
        setItem ( reinterpret_cast<const BrickLink::InvItemList &> ( list ));
}