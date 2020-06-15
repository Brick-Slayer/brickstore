#!/bin/sh

## Copyright (C) 2013-2014 Patrick Brans.  All rights reserved.
##
## This file is part of BrickStock.
## BrickStock is based heavily on BrickStore (http://www.brickforge.de/software/brickstore/)
## by Robert Griebl, Copyright (C) 2004-2008.
##
## This file may be distributed and/or modified under the terms of the GNU 
## General Public License version 2 as published by the Free Software Foundation 
## and appearing in the file LICENSE.GPL included in the packaging of this file.
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
## See http://fsf.org/licensing/licenses/gpl.html for GPL licensing information.

if [ $# != 1 ]; then
  echo "Usage: $0 <release>"
  exit 1
fi

OIFS="$IFS"
IFS="."
set -- $1 
IFS="$OIFS"

cat version.h.in | sed -e "s,\(^#define BRICKSTOCK_MAJOR  *\)[^ ]*$,\1$1,g" \
                       -e "s,\(^#define BRICKSTOCK_MINOR  *\)[^ ]*$,\1$2,g" \
                       -e "s,\(^#define BRICKSTOCK_PATCH  *\)[^ ]*$,\1$3,g" \
>version.h