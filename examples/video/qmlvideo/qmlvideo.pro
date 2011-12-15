TEMPLATE = app
TARGET = qmlvideo

SOURCES += main.cpp
HEADERS += trace.h
RESOURCES += qmlvideo.qrc

qml_folder.source = qml/qmlvideo
qml_folder.target = qml
DEPLOYMENTFOLDERS = qml_folder

images_folder.source = images
images_folder.target =
DEPLOYMENTFOLDERS += images_folder

SNIPPETS_PATH = ../snippets
include($$SNIPPETS_PATH/performancemonitor/performancemonitordeclarative.pri)
performanceItemAddDeployment()

include(qmlapplicationviewer/qmlapplicationviewer.pri)
qtcAddDeployment()
