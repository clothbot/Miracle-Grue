#-------------------------------------------------
#
# Project created by QtCreator 2012
#
#-------------------------------------------------

QT       += core gui

TARGET = bin/miracle_gui
TEMPLATE = app
INSTALLS += target

target.path = /usr/bin

GUI_SRC =  submodule/toolpathviz
include($$GUI_SRC/toolpathviz.pro.inc)

MGL_SRC = src/mgl
include($$MGL_SRC/mgl.pro.inc)

LIBTHING_BASE = submodule/libthing/src/main
include($$LIBTHING_BASE/cpp-qt/Libthing.pro.inc)

JSON_CPP_SRC = submodule/json-cpp
include($$JSON_CPP_SRC/json-cpp.pri)



