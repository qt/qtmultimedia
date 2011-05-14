TEMPLATE = subdirs

module_qtmultimedia_src.subdir = src
module_qtmultimedia_src.target = module-qtmultimedia-src

module_qtmultimedia_demos.subdir = demos
module_qtmultimedia_demos.target = module-qtmultimedia-demos
module_qtmultimedia_demos.depends = module_qtmultimedia_src
!contains(QT_BUILD_PARTS,demos) {
    module_multimedia_demos.CONFIG += no_default_install no_default_target
}


module_qtmultimedia_examples.subdir = examples
module_qtmultimedia_examples.target = module-qtmultimedia-examples
module_qtmultimedia_examples.depends = module_qtmultimedia_src
!contains(QT_BUILD_PARTS,examples) {
    module_multimedia_examples.CONFIG += no_default_install no_default_target
}


module_qtmultimedia_tests.subdir = tests
module_qtmultimedia_tests.target = module-qtmultimedia-tests
module_qtmultimedia_tests.depends = module_qtmultimedia_src
module_qtmultimedia_tests.CONFIG = no_default_install
!contains(QT_BUILD_PARTS,tests):module_qtmultimedia_tests.CONFIG += no_default_target

SUBDIRS += module_qtmultimedia_src \
           module_qtmultimedia_demos \
           module_qtmultimedia_examples \
           module_qtmultimedia_tests \
