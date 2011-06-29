PKG_NAME := qt-mobility
SPECFILE = $(addsuffix .spec, $(PKG_NAME))
YAMLFILE = $(addsuffix .yaml, $(PKG_NAME))

first: custom-help

include /usr/share/meego-packaging-tools/Makefile.common

custom-help: help
	@echo "The above are common MeeGo packaging targets."
	@echo "$(PKG_NAME) also has these additional targets:"
	@echo " spec-git                        Create spec file, with options appropriate for git"
	@echo " build-trunk-i686                Do a local i686 build against Trunk, without using OBS"
	@echo " build-trunk-testing-i686        Do a local i686 build against Trunk:Testing, without using OBS"

spec-git:
	specify --skip-scm --not-download --non-interactive

git-archive: spec-git
	@VER=$$(sed -n -r -e 's|^Version: +||p' $(SPECFILE));     \
	TARGZ=$$(sed -n -r                                        \
	    -e 's|^Source0:.*/([^/]+)$$|\1|'                      \
	    -e 's|%\{name\}|$(PKG_NAME)|'                         \
	    -e "s|%\{version\}|$$VER|"                            \
	    -e 's|.tar.gz||p'                                     \
	$(SPECFILE) );                                            \
	DIR="$$PWD";                                              \
	cd $$(git rev-parse --git-dir) &&                         \
	echo "Creating $$DIR/$$TARGZ.tar.gz ..." &&               \
	git archive HEAD --prefix=$$TARGZ/ --format=tar | gzip > "$$DIR/$$TARGZ.tar.gz"

build-no-obs = sudo build $(SPECFILE) --repository $1 --arch $2

build-no-obs-deps: /usr/bin/build git-archive

build-trunk-i686: build-no-obs-deps
	$(call build-no-obs,http://download.meego.com/live/Trunk/standard/,i686)

build-trunk-testing-i686: build-no-obs-deps
	$(call build-no-obs,http://download.meego.com/live/Trunk:/Testing/standard/,i686)
