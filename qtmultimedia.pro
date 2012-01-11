TEMPLATE = subdirs

module_qtmultimedia_src.subdir = src
module_qtmultimedia_src.target = module-qtmultimedia-src

module_qtmultimedia_examples.subdir = examples
module_qtmultimedia_examples.target = module-qtmultimedia-examples
module_qtmultimedia_examples.depends = module_qtmultimedia_src
!contains(QT_BUILD_PARTS,examples) {
    module_qtmultimedia_examples.CONFIG = no_default_target no_default_install
}

module_qtmultimedia_tests.subdir = tests
module_qtmultimedia_tests.target = module-qtmultimedia-tests
module_qtmultimedia_tests.depends = module_qtmultimedia_src
module_qtmultimedia_tests.CONFIG = no_default_install
!contains(QT_BUILD_PARTS,tests):module_qtmultimedia_tests.CONFIG += no_default_target

module_qtmultimedia_docsnippets.subdir = doc
module_qtmultimedia_docsnippets.target = module-qtmultimedia-doc
module_qtmultimedia_docsnippets.depends = module_qtmultimedia_src
module_qtmultimedia_docsnippets.CONFIG = no_default_install

SUBDIRS += module_qtmultimedia_src \
           module_qtmultimedia_examples \
           module_qtmultimedia_tests

# Doc snippets use widgets
!isEmpty(QT.widgets.name): SUBDIRS += module_qtmultimedia_docsnippets

# for make docs:
include(doc/config/qtmultimedia_doc.pri)
