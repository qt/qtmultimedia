TEMPLATE = subdirs

SUBDIRS+=multimedia.pro

# These autotests consist of things such as static code checks
# which require that the autotest is run on the same machine
# doing the build - i.e. cross-compilation is not allowed.
win32|mac|linux-g++* {
  # NOTE: Disabled until we have established which tests fall into this category
  # !contains(QT_CONFIG,embedded):!maemo5:!maemo6:SUBDIRS+=host.pro
}
