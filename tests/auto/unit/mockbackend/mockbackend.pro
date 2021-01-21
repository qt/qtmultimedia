TEMPLATE = lib
TARGET = QtMultimediaMockBackend
QT = core gui multimedia-private
CONFIG += staticlib

include(audio.pri)
include(capture.pri)
include(common.pri)
include(player.pri)

