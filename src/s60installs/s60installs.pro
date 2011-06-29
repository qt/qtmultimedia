TEMPLATE = subdirs

isEmpty(QT_LIBINFIX):symbian {
    include(../../staticconfig.pri)
    load(data_caging_paths)
    include($$QT_MOBILITY_BUILD_TREE/config.pri)

    SUBDIRS =
    TARGET = "QtMobility"
    TARGET.UID3 = 0x2002ac89

    VERSION = 1.2.0

    vendorinfo = \
        "; Localised Vendor name" \
        "%{\"Nokia\"}" \
        " " \
        "; Unique Vendor name" \
        ":\"Nokia\"" \
        " "
    qtmobilitydeployment.pkg_prerules += vendorinfo


    epoc31 = $$(EPOCROOT31)
    epoc32 = $$(EPOCROOT32)
    epoc50 = $$(EPOCROOT50)
    epoc51 = $$(EPOCROOT51)
    epocS3PS1 = $$(EPOCROOT_S3PS1)
    epocS3PS2 = $$(EPOCROOT_S3PS2)

    # default to EPOCROOT if EPOCROOTxy not defined
    isEmpty(epoc31) {
        EPOCROOT31 = $${EPOCROOT}
    } else {
        EPOCROOT31 = $$(EPOCROOT31)
    }
    isEmpty(epoc32) {
        EPOCROOT32 = $${EPOCROOT}
    }else {
        EPOCROOT32 = $$(EPOCROOT32)
    }
    isEmpty(epoc50) {
        EPOCROOT50 = $${EPOCROOT}
    } else {
        EPOCROOT50 = $$(EPOCROOT50)
    }
    #Epocroot 51 is based on a N97 sdk
    isEmpty(epoc51) {
        EPOCROOT51 = $${EPOCROOT}
    } else {
        EPOCROOT51 = $$(EPOCROOT51)
    }
    isEmpty(epocS3PS1) {
        EPOCROOT_S3PS1 = $${EPOCROOT}
    } else {
        EPOCROOT_S3PS1 = $$(EPOCROOT_S3PS1)
    }
    isEmpty(epocS3PS2) {
        EPOCROOT_S3PS2 = $${EPOCROOT}
    } else {
        EPOCROOT_S3PS2 = $$(EPOCROOT_S3PS2)
    }

    #Symbian^3 and beyond requires special package flags
    #we cannot use S60_VERSION == 5.2 as Qt 4.6.x does not define it yet
    #see $QTDIR/mkspecs/common/symbian/symbian.conf for details
    exists($${EPOCROOT}epoc32/release/winscw/udeb/z/system/install/series60v5.2.sis)|exists($${EPOCROOT}epoc32/data/z/system/install/series60v5.2.sis)|exists($${EPOCROOT}epoc32/release/armv5/lib/libstdcppv5.dso) {
        pkg_version = $$replace(VERSION,"\\.",",")
        qtmobilitydeployment.pkg_prerules += "$${LITERAL_HASH}{\"QtMobility\"},(0x2002AC89),$${pkg_version},TYPE=SA,RU,NR"
    }

    contains(mobility_modules, messaging) {
        qtmobilitydeployment.sources += \
        $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtMessaging.dll

        contains(QT_CONFIG, declarative): {
            qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/declarative_messaging.dll
            pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\messaging\\qmakepluginstubs\\declarative_messaging.qtplugin\"  - \"!:\\resource\\qt\\imports\\QtMobility\\messaging\\declarative_messaging.qtplugin\""
            qmldirs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\messaging\\qmldir\"  - \"!:\\resource\\qt\\imports\\QtMobility\\messaging\\qmldir\""
        }
    }
    
    contains(mobility_modules, connectivity) {
        # QtConnectivity needs to be built and deployed to each Symbian version differently. This
        # is because NFC support is only available from Symbian^3 PS2 onwards. There are also some
        # differences with Bluetooth, hence the Symbian^1 variants.
        #
        # - PS2 build has NFC and Bluetooth support, requires Symbian^3 device with PS2 firmware
        # - PS1 build has Bluetooth support only (NFC is a stub), requires Symbian^3 device with
        #   PS1 firmware
        #   Installing Qt Mobility on a Symbian^3 PS1 device then upgrading device firmware to PS2
        #   will leave Qt Mobility NFC support disabled until Qt Mobility is reinstalled/upgraded.
        # - S60 5.0 build has Bluetooth support only (NFC is a stub)
        # - S60 3.1 and 3.2 build only produces a stub library.
        connectivity = \
            "IF package(0x20022E6D) AND exists(\"z:\\sys\\bin\\nfc.dll\")" \
            "    \"$${EPOCROOT_S3PS2}epoc32/release/$(PLATFORM)/$(TARGET)/QtConnectivity.dll\" - \"!:\\sys\\bin\\QtConnectivity.dll\"" \
            "ELSEIF package(0x20022E6D)" \
            "    \"$${EPOCROOT_S3PS1}epoc32/release/$(PLATFORM)/$(TARGET)/QtConnectivity.dll\" - \"!:\\sys\\bin\\QtConnectivity.dll\"" \
            "ELSEIF package(0x1028315F)" \
            "    \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtConnectivity.dll\" - \"!:\\sys\\bin\\QtConnectivity.dll\"" \
            "ELSEIF package(0x102752AE) OR package(0x102032BE)" \
            "    \"$${EPOCROOT32}epoc32/release/$(PLATFORM)/$(TARGET)/QtConnectivity.dll\" - \"!:\\sys\\bin\\QtConnectivity.dll\"" \
            "ELSE" \
            "    \"$${EPOCROOT}epoc32/release/$(PLATFORM)/$(TARGET)/QtConnectivity.dll\" - \"!:\\sys\\bin\\QtConnectivity.dll\"" \
            "ENDIF"

        contains(QT_CONFIG, declarative): {
            qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/declarative_connectivity.dll
            pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\connectivity\\qmakepluginstubs\\declarative_connectivity.qtplugin\"  - \"!:\\resource\\qt\\imports\\QtMobility\\connectivity\\declarative_connectivity.qtplugin\""
            qmldirs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\connectivity\\qmldir\"  - \"!:\\resource\\qt\\imports\\QtMobility\\connectivity\\qmldir\""
        }


        qtmobilitydeployment.pkg_postrules += connectivity
    }

    contains(mobility_modules, serviceframework) { 
        qtmobilitydeployment.sources += \
        $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtServiceFramework.dll \
        $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qsfwdatabasemanagerserver.exe
        contains(QT_CONFIG, declarative): {
            qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/declarative_serviceframework.dll
            pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\serviceframework\\qmakepluginstubs\\declarative_serviceframework.qtplugin\"  - \"!:\\resource\\qt\\imports\\QtMobility\\serviceframework\\declarative_serviceframework.qtplugin\""
            qmldirs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\serviceframework\\qmldir\"  - \"!:\\resource\\qt\\imports\\QtMobility\\serviceframework\\qmldir\""
        }
    }

    contains(mobility_modules, location) {
        qtmobilitydeployment.sources += $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtLocation.dll
        qtmobilitydeployment.sources += $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtgeoservices_nokia.dll
        qtmobilitydeployment.sources += $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtlandmarks_symbian.dll
        pluginstubs += "\"$$QT_MOBILITY_BUILD_TREE/plugins/geoservices/nokia/qmakepluginstubs/qtgeoservices_nokia.qtplugin\" - \"!:\\resource\\qt\\plugins\\geoservices\\qtgeoservices_nokia.qtplugin\""
        pluginstubs += "\"$$QT_MOBILITY_BUILD_TREE/plugins/landmarks/symbian_landmarks/qmakepluginstubs/qtlandmarks_symbian.qtplugin\" - \"!:\\resource\\qt\\plugins\\landmarks\\qtlandmarks_symbian.qtplugin\""
        contains(QT_CONFIG, declarative): {
            qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/declarative_location.dll
            pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\location\\qmakepluginstubs\\declarative_location.qtplugin\"  - \"!:\\resource\\qt\\imports\\QtMobility\\location\\declarative_location.qtplugin\""
            qmldirs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\location\\qmldir\"  - \"!:\\resource\\qt\\imports\\QtMobility\\location\\qmldir\""
        }
    }

    contains(mobility_modules, systeminfo) { 
        sysinfo = \
            "IF package(0x1028315F)" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtSystemInfo.dll\" - \"!:\\sys\\bin\\QtSystemInfo.dll\"" \
            "ELSEIF package(0x102032BE)" \
            "   \"$${EPOCROOT31}epoc32/release/$(PLATFORM)/$(TARGET)/QtSystemInfo.dll\" - \"!:\\sys\\bin\\QtSystemInfo.dll\"" \
            "ELSEIF package(0x102752AE)" \
            "   \"$${EPOCROOT32}epoc32/release/$(PLATFORM)/$(TARGET)/QtSystemInfo.dll\" - \"!:\\sys\\bin\\QtSystemInfo.dll\"" \
            "ELSE" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtSystemInfo.dll\" - \"!:\\sys\\bin\\QtSystemInfo.dll\"" \
            "ENDIF"

        qtmobilitydeployment.pkg_postrules += sysinfo
        contains(QT_CONFIG, declarative): {
            qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/declarative_systeminfo.dll
            pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\systeminfo\\qmakepluginstubs\\declarative_systeminfo.qtplugin\"  - \"!:\\resource\\qt\\imports\\QtMobility\\systeminfo\\declarative_systeminfo.qtplugin\""
            qmldirs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\systeminfo\\qmldir\"  - \"!:\\resource\\qt\\imports\\QtMobility\\systeminfo\\qmldir\""
        }
    }

    contains(mobility_modules, publishsubscribe) {
        qtmobilitydeployment.sources += \
        $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtPublishSubscribe.dll \
        $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qpspathmapperserver.exe
        contains(QT_CONFIG, declarative) {
            qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/declarative_publishsubscribe.dll
            pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\publishsubscribe\\qmakepluginstubs\\declarative_publishsubscribe.qtplugin\"  - \"!:\\resource\\qt\\imports\\QtMobility\\publishsubscribe\\declarative_publishsubscribe.qtplugin\""
            qmldirs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\publishsubscribe\\qmldir\"  - \"!:\\resource\\qt\\imports\\QtMobility\\publishsubscribe\\qmldir\""
        }
    }

    contains(mobility_modules, versit) {
        qtmobilitydeployment.sources += $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtVersit.dll
        qtmobilitydeployment.sources += $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtversit_backuphandler.dll
        pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\versit\\backuphandler\\qmakepluginstubs\\qtversit_backuphandler.qtplugin\"  - \"!:\\resource\\qt\\plugins\\versit\\qtversit_backuphandler.qtplugin\""

        ## now the versit organizer module - depends on versit and organizer.
        contains(mobility_modules, organizer) {
            qtmobilitydeployment.sources += $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtVersitOrganizer.dll
        }
    }

    contains(mobility_modules, feedback) {
        qtmobilitydeployment.sources += $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtFeedback.dll
        contains(immersion_enabled, yes) {
            qtmobilitydeployment.sources += $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtfeedback_immersion.dll
            pluginstubs += \
                "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\feedback\\immersion\\qmakepluginstubs\\qtfeedback_immersion.qtplugin\"  - \"!:\\resource\\qt\\plugins\\feedback\\qtfeedback_immersion.qtplugin\""
        }

        equals(QT_MAJOR_VERSION, 4) : greaterThan(QT_MINOR_VERSION, 6):contains(mobility_modules,multimedia) {
            qtmobilitydeployment.sources += $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtfeedback_mmk.dll
            pluginstubs += \
                "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\feedback\\mmk\\qmakepluginstubs\\qtfeedback_mmk.qtplugin\"  - \"!:\\resource\\qt\\plugins\\feedback\\qtfeedback_mmk.qtplugin\""
        }

        feedback = \
            "IF package(0x1028315F)" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtfeedback_symbian.dll\" - \"!:\\sys\\bin\\qtfeedback_symbian.dll\"" \
            "ELSEIF package(0x102752AE)" \
            "   \"$${EPOCROOT32}epoc32/release/$(PLATFORM)/$(TARGET)/qtfeedback_symbian.dll\" - \"!:\\sys\\bin\\qtfeedback_symbian.dll\"" \
            "ELSEIF package(0x102032BE)" \
            "   \"$${EPOCROOT31}epoc32/release/$(PLATFORM)/$(TARGET)/qtfeedback_symbian.dll\" - \"!:\\sys\\bin\\qtfeedback_symbian.dll\"" \
            "ELSE" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtfeedback_symbian.dll\" - \"!:\\sys\\bin\\qtfeedback_symbian.dll\"" \
            "ENDIF"

        qtmobilitydeployment.pkg_postrules += feedback

        pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\feedback\\symbian\\qmakepluginstubs\\qtfeedback_symbian.qtplugin\"  - \"!:\\resource\\qt\\plugins\\feedback\\qtfeedback_symbian.qtplugin\""

        contains(QT_CONFIG, declarative): {
            qtmobilitydeployment.sources += \
                $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/declarative_feedback.dll
            pluginstubs += \
                "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\feedback\\qmakepluginstubs\\declarative_feedback.qtplugin\"  - \"!:\\resource\\qt\\imports\\QtMobility\\feedback\\declarative_feedback.qtplugin\""
            qmldirs += \
                "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\feedback\\qmldir\"  - \"!:\\resource\\qt\\imports\\QtMobility\\feedback\\qmldir\""
        }
    }

    contains(mobility_modules, organizer) { 
        qtmobilitydeployment.sources += $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtOrganizer.dll   
        organizer = \
            "IF package(0x1028315F)" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtorganizer_symbian.dll\" - \"!:\\sys\\bin\\qtorganizer_symbian.dll\"" \
            "ELSEIF package(0x102752AE)" \
            "   \"$${EPOCROOT32}epoc32/release/$(PLATFORM)/$(TARGET)/qtorganizer_symbian.dll\" - \"!:\\sys\\bin\\qtorganizer_symbian.dll\"" \
            "ELSEIF package(0x102032BE)" \
            "   \"$${EPOCROOT31}epoc32/release/$(PLATFORM)/$(TARGET)/qtorganizer_symbian.dll\" - \"!:\\sys\\bin\\qtorganizer_symbian.dll\"" \
            "ELSE" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtorganizer_symbian.dll\" - \"!:\\sys\\bin\\qtorganizer_symbian.dll\"" \
            "ENDIF"

        qtmobilitydeployment.pkg_postrules += organizer

        pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\organizer\\symbian\\qmakepluginstubs\\qtorganizer_symbian.qtplugin\"  - \"!:\\resource\\qt\\plugins\\organizer\\qtorganizer_symbian.qtplugin\""
     contains(QT_CONFIG, declarative):contains(mobility_modules,versit)  {
            qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/declarative_organizer.dll
            pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\organizer\\qmakepluginstubs\\declarative_organizer.qtplugin\"  - \"!:\\resource\\qt\\imports\\QtMobility\\organizer\\declarative_organizer.qtplugin\""
            qmldirs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\organizer\\qmldir\"  - \"!:\\resource\\qt\\imports\\QtMobility\\organizer\\qmldir\""
        }
    }

    contains(mobility_modules, gallery) {
        #QDocumentGallery on S60/Symbian relies on MetaDataSystem. There exists few different versions of it and we must
        #check what version is currently installed on devices. Check is made a with known dlls.
        #Installation has these preconditions:
        # QDocumentGallery built against EPOCROOT50 has mds 2.0 libs in place (3.1/3.2/5.0)
        # QDocumentGallery built against EPOCROOT51 has mds 2.5 libs in place (Symbian^3 and N97)
        # QDocumentGallery built against EPOCROOT32 has no mds libs at all (stub implementation, api only)
        # Also if checked mds library is present on c-drive then also check whether mds is installed
        gallerymdscheck = \
            "if exists(\"z:\sys\bin\locationutility.dll\")" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtGallery.dll\" - \"!:\\sys\\bin\\QtGallery.dll\"" \
            "elseif exists(\"c:\sys\bin\locationutility.dll\") AND package(0x200009F5)" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtGallery.dll\" - \"!:\\sys\\bin\\QtGallery.dll\"" \
            "elseif exists(\"z:\sys\bin\locationmanagerserver.exe\")" \
            "   \"$${EPOCROOT51}epoc32/release/$(PLATFORM)/$(TARGET)/QtGallery.dll\" - \"!:\\sys\\bin\\QtGallery.dll\"" \
            "elseif exists(\"c:\sys\bin\locationmanagerserver.exe\") AND package(0x200009F5)" \
            "   \"$${EPOCROOT51}epoc32/release/$(PLATFORM)/$(TARGET)/QtGallery.dll\" - \"!:\\sys\\bin\\QtGallery.dll\"" \
            "else" \
            "   \"$${EPOCROOT32}epoc32/release/$(PLATFORM)/$(TARGET)/QtGallery.dll\" - \"!:\\sys\\bin\\QtGallery.dll\"" \
            "endif"

        qtmobilitydeployment.pkg_postrules += gallerymdscheck

        # QDocumentGallery QML plugin
        contains(QT_CONFIG, declarative): {
            qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/declarative_gallery.dll
            pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\gallery\\qmakepluginstubs\\declarative_gallery.qtplugin\"  - \"!:\\resource\\qt\\imports\\QtMobility\\gallery\\declarative_gallery.qtplugin\""
            qmldirs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\gallery\\qmldir\"  - \"!:\\resource\\qt\\imports\\QtMobility\\gallery\\qmldir\""
        }
    }

    contains(mobility_modules, bearer) {
        bearer = \
            "IF package(0x1028315F)" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtBearer.dll\" - \"!:\\sys\\bin\\QtBearer.dll\"" \
            "ELSEIF package(0x102752AE)" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtBearer.dll\" - \"!:\\sys\\bin\\QtBearer.dll\"" \
            "ELSEIF package(0x102032BE)" \
            "   \"$${EPOCROOT31}epoc32/release/$(PLATFORM)/$(TARGET)/QtBearer.dll\" - \"!:\\sys\\bin\\QtBearer.dll\"" \
            "ELSE" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtBearer.dll\" - \"!:\\sys\\bin\\QtBearer.dll\"" \
            "ENDIF"

        qtmobilitydeployment.pkg_postrules += bearer
    }
    
    contains(mobility_modules, bearer) {
            !contains(MOBILITY_SD_MCL_BUILD, yes):exists($${EPOCROOT}epoc32/release/winscw/udeb/z/system/install/series60v5.2.sis)|exists($${EPOCROOT}epoc32/data/z/system/install/series60v5.2.sis)|exists($${EPOCROOT}epoc32/release/armv5/lib/libstdcppv5.dso) {
            bearer10_0 = \ 
	    "IF package(0x1028315F)" \
                "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtBearer{000a0000}.dll\" - \"!:\\sys\\bin\\QtBearer{000a0000}.dll\"" \
                "ELSEIF package(0x102752AE)" \
                "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtBearer{000a0000}.dll\" - \"!:\\sys\\bin\\QtBearer{000a0000}.dll\"" \
                "ELSEIF package(0x102032BE)" \
                "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtBearer{000a0000}.dll\" - \"!:\\sys\\bin\\QtBearer{000a0000}.dll\"" \
                "ELSE" \
                "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtBearer{000a0000}.dll\" - \"!:\\sys\\bin\\QtBearer{000a0000}.dll\"" \
                "ENDIF"
            qtmobilitydeployment.pkg_postrules += bearer10_0
        }
    }

    contains(mobility_modules, contacts) {

        qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtContacts.dll
        contains(mobility_modules,serviceframework):qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtcontacts_serviceactionmanager.dll

        contacts = \
            "IF package(0x1028315F)" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtcontacts_symbian.dll\" - \"!:\\sys\\bin\\qtcontacts_symbian.dll\"" \
            "ELSEIF package(0x102752AE)" \
            "   \"$${EPOCROOT32}epoc32/release/$(PLATFORM)/$(TARGET)/qtcontacts_symbian.dll\" - \"!:\\sys\\bin\\qtcontacts_symbian.dll\"" \
            "ELSEIF package(0x102032BE)" \
            "   \"$${EPOCROOT31}epoc32/release/$(PLATFORM)/$(TARGET)/qtcontacts_symbian.dll\" - \"!:\\sys\\bin\\qtcontacts_symbian.dll\"" \
            "ELSE" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtcontacts_symbian.dll\" - \"!:\\sys\\bin\\qtcontacts_symbian.dll\"" \
            "ENDIF"

        qtmobilitydeployment.pkg_postrules += contacts

        pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE/plugins/contacts/symbian/plugin/qmakepluginstubs/qtcontacts_symbian.qtplugin\"  - \"!:\\resource\\qt\\plugins\\contacts\\qtcontacts_symbian.qtplugin\""
        contains(mobility_modules,serviceframework):pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE/plugins/contacts/serviceactionmanager/qmakepluginstubs/qtcontacts_serviceactionmanager.qtplugin\"  - \"!:\\resource\\qt\\plugins\\contacts\\qtcontacts_serviceactionmanager.qtplugin\""

        contains(symbiancntsim_enabled, yes) {
            pluginstubs += \
                "\"$$QT_MOBILITY_BUILD_TREE/plugins/contacts/symbiansim/qmakepluginstubs/qtcontacts_symbiansim.qtplugin\"  - \"!:\\resource\\qt\\plugins\\contacts\\qtcontacts_symbiansim.qtplugin\""

            symbiancntsim = \
                "IF package(0x1028315F)" \
                "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtcontacts_symbiansim.dll\" - \"!:\\sys\\bin\\qtcontacts_symbiansim.dll\"" \
                "ELSEIF package(0x102752AE)" \
                "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtcontacts_symbiansim.dll\" - \"!:\\sys\\bin\\qtcontacts_symbiansim.dll\"" \
                "ELSEIF package(0x102032BE)" \
                "   \"$${EPOCROOT31}epoc32/release/$(PLATFORM)/$(TARGET)/qtcontacts_symbiansim.dll\" - \"!:\\sys\\bin\\qtcontacts_symbiansim.dll\"" \
                "ELSE" \
                "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtcontacts_symbiansim.dll\" - \"!:\\sys\\bin\\qtcontacts_symbiansim.dll\"" \
                "ENDIF"

            qtmobilitydeployment.pkg_postrules += symbiancntsim
        }
     contains(QT_CONFIG, declarative):contains(mobility_modules,versit)  {
            qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/declarative_contacts.dll
            pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\contacts\\qmakepluginstubs\\declarative_contacts.qtplugin\"  - \"!:\\resource\\qt\\imports\\QtMobility\\contacts\\declarative_contacts.qtplugin\""
            qmldirs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\contacts\\qmldir\"  - \"!:\\resource\\qt\\imports\\QtMobility\\contacts\\qmldir\""
        }
    }

    contains(mobility_modules, multimedia) {

        qtmobilitydeployment.sources += \
            $$(EPOCROOT50)epoc32/release/$(PLATFORM)/$(TARGET)/QtMultimediaKit.dll \
            $$(EPOCROOT50)epoc32/release/$(PLATFORM)/$(TARGET)/qtmultimediakit_m3u.dll

        pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE/plugins/multimedia/m3u/qmakepluginstubs/qtmultimediakit_m3u.qtplugin\"     - \"!:\\resource\\qt\\plugins\\playlistformats\\qtmultimediakit_m3u.qtplugin\""

        contains(openmaxal_symbian_enabled, yes) {
            openmax = \
                "\"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtmultimediakit_openmaxalengine.dll\" - \"!:\\sys\\bin\\qtmultimediakit_openmaxalengine.dll\""

            qtmobilitydeployment.pkg_postrules += openmax

            pluginstubs += \
                    "\"$$QT_MOBILITY_BUILD_TREE/plugins/multimedia/symbian/openmaxal/qmakepluginstubs/qtmultimediakit_openmaxalengine.qtplugin\" - \"!:\\resource\\qt\\plugins\\mediaservice\\qtmultimediakit_openmaxalengine.qtplugin\""
        } else {

            multimedia = \
                "IF package(0x1028315F)" \
                "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtmultimediakit_mmfengine.dll\" - \"!:\\sys\\bin\\qtmultimediakit_mmfengine.dll\"" \
                "ELSEIF package(0x102752AE)" \
                "   \"$${EPOCROOT32}epoc32/release/$(PLATFORM)/$(TARGET)/qtmultimediakit_mmfengine.dll\" - \"!:\\sys\\bin\\qtmultimediakit_mmfengine.dll\"" \
                "ELSEIF package(0x102032BE)" \
                "   \"$${EPOCROOT31}epoc32/release/$(PLATFORM)/$(TARGET)/qtmultimediakit_mmfengine.dll\" - \"!:\\sys\\bin\\qtmultimediakit_mmfengine.dll\"" \
                "ELSEIF package(0x20022E6D)" \
                "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtmultimediakit_mmfengine.dll\" - \"!:\\sys\\bin\\qtmultimediakit_mmfengine.dll\"" \
                "ENDIF"

            qtmobilitydeployment.pkg_postrules += multimedia

            pluginstubs += \
                "IF package(0x1028315F)" \
                    "\"$$QT_MOBILITY_BUILD_TREE/plugins/multimedia/symbian/mmf/qmakepluginstubs/qtmultimediakit_mmfengine.qtplugin\" - \"!:\\resource\\qt\\plugins\\mediaservice\\qtmultimediakit_mmfengine.qtplugin\"" \
                "ELSEIF package(0x102752AE)" \
                    "\"$$QT_MOBILITY_BUILD_TREE/plugins/multimedia/symbian/mmf/qmakepluginstubs/qtmultimediakit_mmfengine.qtplugin\" - \"!:\\resource\\qt\\plugins\\mediaservice\\qtmultimediakit_mmfengine.qtplugin\"" \
                "ELSEIF package(0x102032BE)" \
                    "\"$$QT_MOBILITY_BUILD_TREE/plugins/multimedia/symbian/mmf/qmakepluginstubs/qtmultimediakit_mmfengine.qtplugin\" - \"!:\\resource\\qt\\plugins\\mediaservice\\qtmultimediakit_mmfengine.qtplugin\"" \
                "ELSEIF package(0x20022E6D)" \
                    "\"$$QT_MOBILITY_BUILD_TREE/plugins/multimedia/symbian/mmf/qmakepluginstubs/qtmultimediakit_mmfengine.qtplugin\" - \"!:\\resource\\qt\\plugins\\mediaservice\\qtmultimediakit_mmfengine.qtplugin\"" \
                "ENDIF"
        }

        camera = \
            "IF package(0x1028315F)" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtmultimediakit_ecamengine.dll\" - \"!:\\sys\\bin\\qtmultimediakit_ecamengine.dll\"" \
            "ELSEIF package(0x102752AE)" \
            "   \"$${EPOCROOT32}epoc32/release/$(PLATFORM)/$(TARGET)/qtmultimediakit_ecamengine.dll\" - \"!:\\sys\\bin\\qtmultimediakit_ecamengine.dll\"" \
            "ELSEIF package(0x102032BE)" \
            "   \"$${EPOCROOT31}epoc32/release/$(PLATFORM)/$(TARGET)/qtmultimediakit_ecamengine.dll\" - \"!:\\sys\\bin\\qtmultimediakit_ecamengine.dll\"" \
            "ELSEIF package(0x20022E6D)" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtmultimediakit_ecamengine.dll\" - \"!:\\sys\\bin\\qtmultimediakit_ecamengine.dll\"" \
            "ENDIF"

        qtmobilitydeployment.pkg_postrules += camera

        pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE/plugins/multimedia/symbian/ecam/qmakepluginstubs/qtmultimediakit_ecamengine.qtplugin\" - \"!:\\resource\\qt\\plugins\\mediaservice\\qtmultimediakit_ecamengine.qtplugin\""


    contains(QT_CONFIG, declarative): {
            qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/declarative_multimedia.dll
            pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\multimedia\\qmakepluginstubs\\declarative_multimedia.qtplugin\"  - \"!:\\resource\\qt\\imports\\QtMultimediaKit\\declarative_multimedia.qtplugin\""
            qmldirs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\multimedia\\qmldir\"  - \"!:\\resource\\qt\\imports\\QtMultimediaKit\\qmldir\""
        }
    }

    contains(mobility_modules, sensors) {

        equals(sensors_symbian_enabled,yes) {
            sensorplugin=symbian
        } else:equals(sensors_s60_31_enabled,yes) {
            sensorplugin=s60_sensor_api
        } else {
            error("Must have a Symbian sensor backend available")
        }

        qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/QtSensors.dll

        sensors = \
            "IF package(0x1028315F)" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtsensors_sym.dll\" - \"!:\\sys\\bin\\qtsensors_sym.dll\"" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtsensors_generic.dll\" - \"!:\\sys\\bin\\qtsensors_generic.dll\"" \
            "ELSEIF package(0x102752AE)" \
            "   \"$${EPOCROOT32}epoc32/release/$(PLATFORM)/$(TARGET)/qtsensors_sym.dll\" - \"!:\\sys\\bin\\qtsensors_sym.dll\"" \
            "   \"$${EPOCROOT32}epoc32/release/$(PLATFORM)/$(TARGET)/qtsensors_generic.dll\" - \"!:\\sys\\bin\\qtsensors_generic.dll\"" \
            "ELSEIF package(0x102032BE)" \
            "   \"$${EPOCROOT31}epoc32/release/$(PLATFORM)/$(TARGET)/qtsensors_sym.dll\" - \"!:\\sys\\bin\\qtsensors_sym.dll\"" \
            "   \"$${EPOCROOT31}epoc32/release/$(PLATFORM)/$(TARGET)/qtsensors_generic.dll\" - \"!:\\sys\\bin\\qtsensors_generic.dll\"" \
            "ELSE" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtsensors_sym.dll\" - \"!:\\sys\\bin\\qtsensors_sym.dll\"" \
            "   \"$${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/qtsensors_generic.dll\" - \"!:\\sys\\bin\\qtsensors_generic.dll\"" \
            "ENDIF"

        pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE/plugins/sensors/$$sensorplugin/qmakepluginstubs/qtsensors_sym.qtplugin\"  - \"!:\\resource\\qt\\plugins\\sensors\\qtsensors_sym.qtplugin\"" \
            "\"$$QT_MOBILITY_BUILD_TREE/plugins/sensors/generic/qmakepluginstubs/qtsensors_generic.qtplugin\"  - \"!:\\resource\\qt\\plugins\\sensors\\qtsensors_generic.qtplugin\""

        !isEmpty(sensors):qtmobilitydeployment.pkg_postrules += sensors
        contains(QT_CONFIG, declarative): {
            qtmobilitydeployment.sources += \
            $${EPOCROOT50}epoc32/release/$(PLATFORM)/$(TARGET)/declarative_sensors.dll
            pluginstubs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\sensors\\qmakepluginstubs\\declarative_sensors.qtplugin\"  - \"!:\\resource\\qt\\imports\\QtMobility\\sensors\\declarative_sensors.qtplugin\""
            qmldirs += \
            "\"$$QT_MOBILITY_BUILD_TREE\\plugins\\declarative\\sensors\\qmldir\"  - \"!:\\resource\\qt\\imports\\QtMobility\\sensors\\qmldir\""
        }
    }

    !isEmpty(pluginstubs):qtmobilitydeployment.pkg_postrules += pluginstubs
    !isEmpty(qmldirs):qtmobilitydeployment.pkg_postrules += qmldirs
 
    qtmobilitydeployment.path = /sys/bin

    # Support backup and restore for QtMobility libraries and applications
    mobilitybackup.sources = backup_registration.xml
    mobilitybackup.path = c:/private/10202d56/import/packages/$$replace(TARGET.UID3, 0x,)

    DEPLOYMENT += qtmobilitydeployment\
                mobilitybackup
} else {
    message(Deployment of infixed library names not supported)
}
