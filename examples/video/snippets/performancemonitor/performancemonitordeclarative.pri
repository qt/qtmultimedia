include($$PWD/../frequencymonitor/frequencymonitordeclarative.pri)
include($$PWD/performancemonitor.pri)

HEADERS += $$PWD/performancemonitordeclarative.h
SOURCES += $$PWD/performancemonitordeclarative.cpp

PERFORMANCE_ROOT = $$PWD

defineTest(performanceItemAddDeployment) {
    frequencyItemAddDeployment()

    symbian: performanceitem_folder.source = $$PWD/$$SNIPPETS_PATH/performancemonitor/qml/performancemonitor
    else: performanceitem_folder.source = $$SNIPPETS_PATH/performancemonitor/qml/performancemonitor
    performanceitem_folder.target = qml
    DEPLOYMENTFOLDERS += performanceitem_folder

    export(performanceitem_folder.source)
    export(performanceitem_folder.target)
    export(DEPLOYMENTFOLDERS)
}
