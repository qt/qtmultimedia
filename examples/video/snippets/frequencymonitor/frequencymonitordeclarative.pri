include($$PWD/frequencymonitor.pri)
QT += qml
SOURCES += $$PWD/frequencymonitordeclarative.cpp

defineTest(frequencyItemAddDeployment) {
    symbian: frequencyitem_folder.source = $$PWD/$$SNIPPETS_PATH/frequencymonitor/qml/frequencymonitor
    else: frequencyitem_folder.source = $$SNIPPETS_PATH/frequencymonitor/qml/frequencymonitor
    frequencyitem_folder.target = qml
    DEPLOYMENTFOLDERS += frequencyitem_folder

    export(frequencyitem_folder.source)
    export(frequencyitem_folder.target)
    export(DEPLOYMENTFOLDERS)
}

