%modules = ( # path to module name map
    "QtMultimedia" => "$basedir/src/multimedia",
    "QtMultimediaWidgets" => "$basedir/src/multimediawidgets",
);

%moduleheaders = ( # restrict the module headers to those found in relative path
);

%classnames = (
    "qtmultimediaversion.h" => "QtMultimediaVersion",
    "qtmultimediawidgetsversion.h" => "QtMultimediaWidgetsVersion",
);

%mastercontent = (
    "gui" => "#include <QtGui/QtGui>\n",
    "network" => "#include <QtNetwork/QtNetwork>\n",
    "core" => "#include <QtCore/QtCore>\n",
);

%modulepris = (
    "QtMultimedia" => "$basedir/modules/qt_multimedia.pri",
    "QtMultimediaWidgets" => "$basedir/modules/qt_multimediawidgets.pri",
);

# Module dependencies.
# Every module that is required to build this module should have one entry.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - any git symbolic ref resolvable from the module's repository (e.g. "refs/heads/master" to track master branch)
#
%dependencies = (
    "qtbase" => "refs/heads/master",
    "qtsvg" => "refs/heads/master",
    "qtxmlpatterns" => "refs/heads/master",
    "qtdeclarative" => "refs/heads/master",
);
# Compile tests
%configtests = (
    # Windows tests
    "directshow" => {},
    "wmsdk" => {},
    "wmp" => {},
    "wmf" => {},
    "evr" => {},

    # Linux tests
    "alsa" => {},
    "gstreamer" => {},
    "gstreamer_photography" => {},
    "gstreamer_appsrc" => {},
    "pulseaudio" => {},
    "resourcepolicy" => {},
);
