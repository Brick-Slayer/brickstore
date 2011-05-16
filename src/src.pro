## Copyright (C) 2004-2008 Robert Griebl.  All rights reserved.
##
## This file is part of BrickStore.
##
## This file may be distributed and/or modified under the terms of the GNU
## General Public License version 2 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of this file.
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
## See http://fsf.org/licensing/licenses/gpl.html for GPL licensing information.

include(../release.pri)

TEMPLATE     = app
CONFIG      *= warn_on thread qt
# CONFIG      *= modeltest
QT          *= core gui xml network script

static:QTPLUGIN  *= qjpeg qgif
static:DEFINES   += STATIC_QT_BUILD

TARGET            = BrickStore
unix:!macx:TARGET = brickstore

LANGUAGES    = de fr nl sl
RESOURCES   += brickstore.qrc

DEFINES += BRICKSTORE_MAJOR=$$RELEASE_MAJOR BRICKSTORE_MINOR=$$RELEASE_MINOR BRICKSTORE_PATCH=$$RELEASE_PATCH
DEFINES += __USER__=\"$$RELEASE_USER\" __HOST__=\"$$RELEASE_HOST\"

SOURCES += \
  appearsinwidget.cpp \
  application.cpp \
  config.cpp \
  document.cpp \
  documentdelegate.cpp \
  framework.cpp \
  itemdetailpopup.cpp \
  main.cpp \
  picturewidget.cpp \
  priceguidewidget.cpp \
  rebuilddatabase.cpp \
  report.cpp \
  reportobjects.cpp \
  selectcolor.cpp \
  selectitem.cpp \
  splash.cpp \
  taskwidgets.cpp \
  window.cpp \

HEADERS += \
  appearsinwidget.h \
  application.h \
  checkforupdates.h \
  config.h \
  document.h \
  document_p.h \
  documentdelegate.h \
  framework.h \
  import.h \
  itemdetailpopup.h \
  picturewidget.h \
  priceguidewidget.h \
  rebuilddatabase.h \
  ref.h \
  report.h \
  report_p.h \
  reportobjects.h \
  selectcolor.h \
  selectitem.h \
  splash.h \
  taskwidgets.h \
  updatedatabase.h \
  window.h \
  version.h \

FORMS = \
  additemdialog.ui \
  consolidateitemsdialog.ui \
  importinventorydialog.ui \
  importorderdialog.ui \
  incdecpricesdialog.ui \
  incompleteitemdialog.ui \
  informationdialog.ui \
  selectcolordialog.ui \
  selectdocumentdialog.ui \
  selectitemdialog.ui \
  selectreportdialog.ui \
  settingsdialog.ui \
  settopriceguidedialog.ui \

HEADERS += $$replace(FORMS, '\\.ui', '.h')
SOURCES += $$replace(FORMS, '\\.ui', '.cpp')

include('utility/utility.pri')
include('bricklink/bricklink.pri')
include('ldraw/ldraw.pri')
include('lzma/lzma.pri')
modeltest:debug:include('modeltest/modeltest.pri')

TRANSLATIONS = $$replace(LANGUAGES, '$', '.ts')
TRANSLATIONS = $$replace(TRANSLATIONS, '^', 'translations/brickstore_')

#
# Windows specific
#

win32 {
  CONFIG  += windows
  RC_FILE  = $$PWD/../win32/brickstore.rc

  win32-msvc* {
    QMAKE_CXXFLAGS_DEBUG   += /Od /GL-
    QMAKE_CXXFLAGS_RELEASE += /O2 /GL

    LIBS += user32.lib advapi32.lib
  }
  
  win32-msvc2005 {
     DEFINES += _CRT_SECURE_NO_DEPRECATE

     # QMAKE_LFLAGS_WINDOWS += "/MANIFEST:NO"
     release:QMAKE_LFLAGS_WINDOWS += "/LTCG"

     QMAKE_CXXFLAGS_DEBUG   += /EHc- /EHs- /GR-
     QMAKE_CXXFLAGS_RELEASE += /EHc- /EHs- /GR-
  }
}


#
# Unix specific
#

unix {
  debug:OBJECTS_DIR   = $$OUT_PWD/.obj/debug
  release:OBJECTS_DIR = $$OUT_PWD/.obj/release
  debug:MOC_DIR       = $$OUT_PWD/.moc/debug
  release:MOC_DIR     = $$OUT_PWD/.moc/release
  UI_DIR              = $$OUT_PWD/.uic
  RCC_DIR             = $$OUT_PWD/.rcc
}


#
# Unix/X11 specific
#

unix:!macx {
  CONFIG += x11

  isEmpty( PREFIX ):PREFIX = /usr/local
  target.path = $$PREFIX/bin
  INSTALLS += target
}


#
# Mac OS X specific
#

macx {
  # CONFIG += x86

  QMAKE_INFO_PLIST = $$PWD/../macx/Info.plist
  bundle_icons.files = $$system(find $$PWD/../macx/Resources/ -name '*.icns')
  bundle_icons.path = Contents/Resources
  bundle_locversions.files = $$system(find $$PWD/../macx/Resources/ -name '*.lproj')
  bundle_locversions.path = Contents/Resources
  QMAKE_BUNDLE_DATA += bundle_icons bundle_locversions
}