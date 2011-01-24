%modules = ( # path to module name map
    "QtMultimedia" => "$basedir/src/multimedia",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
);
%classnames = (
);
%mastercontent = (
    "gui" => "#include <QtGui/QtGui>\n",
    "core" => "#include <QtCore/QtCore>\n",
);
%modulepris = (
    "QtMultimedia" => "$basedir/modules/qt_multimedia.pri",
);
