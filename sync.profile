%modules = ( # path to module name map
    "QtMultimediaKit" => "$basedir/src/multimediakit",
);

%moduleheaders = ( # restrict the module headers to those found in relative path
);

%classnames = (
    "qtmultimediakitversion.h" => "QtMultimediaKitVersion",
);

%mastercontent = (
    "gui" => "#include <QtGui/QtGui>\n",
    "network" => "#include <QtNetwork/QtNetwork>\n",
    "core" => "#include <QtCore/QtCore>\n",
);

%modulepris = (
    "QtMultimediaKit" => "$basedir/modules/qt_multimediakit.pri",
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
    "qtscript" => "refs/heads/master",
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
    "gstreamer_photography" => {},
    "gstreamer_appsrc" => {},
    "pulseaudio" => {},
);
