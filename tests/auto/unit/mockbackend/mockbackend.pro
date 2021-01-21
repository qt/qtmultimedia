TEMPLATE = lib
TARGET = QtMultimediaMockBackend
QT = core gui multimedia-private
CONFIG += staticlib

include(audio.pri)
include(capture.pri)
include(common.pri)
include(player.pri)

SOURCES += qmockdevicemanager.cpp qmockintegration.cpp
HEADERS += qmockdevicemanager_p.h qmockintegration_p.h
