%modules = ( # path to module name map
    "QtMultimedia" => "$basedir/src/multimedia",
    "QtMultimediaWidgets" => "$basedir/src/multimediawidgets",
    "QtMultimediaQuick_p" => "$basedir/src/qtmultimediaquicktools",
);

%moduleheaders = ( # restrict the module headers to those found in relative path
);

%classnames = (
    "qaudio.h" => "QAudio",
    "qmediametadata.h" => "QMediaMetaData",
    "qmultimedia.h" => "QMultimedia"
);

# Module dependencies.
# Every module that is required to build this module should have one entry.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - any git symbolic ref resolvable from the module's repository (e.g. "refs/heads/master" to track master branch)
#
%dependencies = (
    "qtbase" => "refs/heads/release",
    "qtxmlpatterns" => "refs/heads/release",
    "qtdeclarative" => "refs/heads/release",
    "qtjsbackend" => "refs/heads/release",
);
