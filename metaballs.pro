QT += core
QT += gui
QT += widgets

CONFIG += c++11

TARGET = metaballs
CONFIG -= app_bundle

#QMAKE_CXXFLAGS_RELEASE -= -O2
#QMAKE_CXXFLAGS_RELEASE += -O3

TEMPLATE = app

SOURCES += main.cpp

HEADERS += \
    scalarfield.h \
    inlinemath.h \
    renderer.h
