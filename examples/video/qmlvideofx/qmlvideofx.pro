TEMPLATE = app
TARGET = qmlvideofx

SOURCES += filereader.cpp \
           main.cpp
HEADERS += filereader.h \
           trace.h
RESOURCES += qmlvideofx.qrc

qml_folder.source = qml/qmlvideofx
qml_folder.target = qml
DEPLOYMENTFOLDERS += qml_folder

images_folder.source = images
images_folder.target =
DEPLOYMENTFOLDERS += images_folder

shaders_folder.source = shaders
shaders_folder.target =
DEPLOYMENTFOLDERS += shaders_folder

SNIPPETS_PATH = ../snippets
include($$SNIPPETS_PATH/performancemonitor/performancemonitordeclarative.pri)
performanceItemAddDeployment()

maemo6: {
    DEFINES += SMALL_SCREEN_LAYOUT
    DEFINES += SMALL_SCREEN_PHYSICAL
}

include(qmlapplicationviewer/qmlapplicationviewer.pri)
qtcAddDeployment()
