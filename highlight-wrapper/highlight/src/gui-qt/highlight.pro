
TARGET = highlight-gui
TEMPLATE = app
INCLUDEPATH += . ../include
QT += core gui

QT += widgets

PRECOMPILED_HEADER = precomp.h
DEFINES += O2 QT

SOURCES += main.cpp \
    mainwindow.cpp \
    io_report.cpp \
    showtextfile.cpp \
    syntax_chooser.cpp

HEADERS += mainwindow.h \
    precomp.h \
    io_report.h \
    showtextfile.h \
    syntax_chooser.h

FORMS += mainwindow.ui \
    io_report.ui \
    showtextfile.ui \
    syntax_chooser.ui

RESOURCES = highlight-gui.qrc
TRANSLATIONS = \
    highlight_de_DE.ts \
    highlight_es_ES.ts \
    highlight_cs_CZ.ts \
    highlight_zh_CN.ts \
    highlight_it_IT.ts \
    highlight_fr_FR.ts \
    highlight_bg_BG.ts \
    highlight_ja_JP.ts

QMAKE_CXXFLAGS += -std=c++17

# If Lua 5.2 is not default on your system yet you have to omit 5.1 here:
unix {
    DESTDIR = ../
    LIBS += -L.. -lhighlight
    LIBS += -llua
    CONFIG += link_pkgconfig
    PKGCONFIG += lua

    # to make it run within Qt Creator
    !contains(DEFINES, HL_DATA_DIR.*) {
        DEFINES+=HL_DATA_DIR=\\\"/usr/share/highlight/\\\"
    }
    !contains(DEFINES, HL_CONFIG_DIR.*) {
        DEFINES+=HL_CONFIG_DIR=\\\"/etc/highlight/\\\"
    }
    !contains(DEFINES, HL_DOC_DIR.*) {
        DEFINES+=HL_DOC_DIR=\\\"/usr/share/doc/highlight/\\\"
    }
}

win32 {
    DESTDIR = ../..

    INCLUDEPATH += F:\libs\lua_5.4.0\
    INCLUDEPATH += ../../include
    INCLUDEPATH += F:/libs/boost_1_82_0

    DEFINES += WIN32
    DEFINES -= NOCRYPT
    CONFIG += static
    LIBS += -LF:\git\highlight\src -lhighlight

    contains(QT_ARCH, i386) {
        LIBS += -LF:\libs\lua_5.4.0\x32 -llua
    } else {
        LIBS += -LF:\libs\lua_5.4.0\x64 -llua
    }

    RC_FILE = highlight-gui.rc
    QMAKE_POST_LINK = F:\upx393w\upx.exe --best --force F:\git\highlight\highlight-gui.exe
}


macx-clang|macx-clang-arm64  {
    QMAKE_CC = clang
    QMAKE_CXX = clang++
    INCLUDEPATH+=/usr/local/Cellar/lua/5.4.4_1/include/
    INCLUDEPATH += ../../include
    INCLUDEPATH+=/usr/local/Cellar/boost/1.85.0/include

    QMAKE_MACOSX_DEPLOYMENT_TARGET=14.0
    CONFIG += app_bundle
    LIBS += -L.. -lhighlight
    LIBS += -L/Users/andresimon/MyProjects/lua-5.4.1/src -llua

    ICON = $${PWD}/highlight.icns
}
