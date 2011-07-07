%modules = ( # path to module name map
    "QtMultimedia" => "$basedir/src/multimedia",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
);
%classnames = (
    "qtmultimediaversion.h" => "QtMultimediaVersion",
);
%mastercontent = (
    "gui" => "#include <QtGui/QtGui>\n",
    "core" => "#include <QtCore/QtCore>\n",
);
%modulepris = (
    "QtMultimedia" => "$basedir/modules/qt_multimedia.pri",
);
# Module dependencies.
# Every module that is required to build this module should have one entry.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - any git symbolic ref resolvable from the module's repository (e.g. "refs/heads/master" to track master branch)
#
%dependencies = (
    "qtbase" => "refs/heads/master",
);
