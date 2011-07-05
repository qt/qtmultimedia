TEMPLATE = subdirs

module_qtmultimediakit_src.subdir = src
module_qtmultimediakit_src.target = module-qtmultimediakit-src

module_qtmultimediakit_demos.subdir = demos
module_qtmultimediakit_demos.target = module-qtmultimediakit-demos
module_qtmultimediakit_demos.depends = module_qtmultimediakit_src
!contains(QT_BUILD_PARTS,demos) {
    module_qtmultimediakit_demos.CONFIG = no_default_target no_default_install
}

module_qtmultimediakit_examples.subdir = examples
module_qtmultimediakit_examples.target = module-qtmultimediakit-examples
module_qtmultimediakit_examples.depends = module_qtmultimediakit_src
!contains(QT_BUILD_PARTS,examples) {
    module_qtmultimediakit_examples.CONFIG = no_default_target no_default_install
}

module_qtmultimediakit_tests.subdir = tests
module_qtmultimediakit_tests.target = module-qtmultimediakit-tests
module_qtmultimediakit_tests.depends = module_qtmultimediakit_src
module_qtmultimediakit_tests.CONFIG = no_default_target no_default_install

SUBDIRS += module_qtmultimediakit_src \
           module_qtmultimediakit_demos \
           module_qtmultimediakit_examples \
           module_qtmultimediakit_tests \

