/* Copyright (C) 2004-2008 Robert Griebl.  All rights reserved.
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
#ifndef __CSPINNER_H__
#define __CSPINNER_H__

#include <qwidget.h>

class QPixmap;


class CSpinner : public QWidget {
	Q_OBJECT
public:
	CSpinner ( QWidget *parent, const char *name = 0, WFlags fl = 0 );
	virtual ~CSpinner ( );

	void setPixmap ( const QPixmap &p );

	void start ( );
	void stop ( );
	bool isActive ( ) const;

	virtual QSize minimumSizeHint ( ) const;

public slots:
	void setActive ( bool );

protected:
	virtual void timerEvent ( QTimerEvent * );
	virtual void paintEvent ( QPaintEvent * );

private:
	int      m_width;
	int      m_count;
	QPixmap *m_pixmap;
	int      m_step;
	int      m_timer_id;
};

#endif
