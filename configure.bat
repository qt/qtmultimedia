:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
:: All rights reserved.
:: Contact: Nokia Corporation (qt-info@nokia.com)
::
:: This file is part of the Qt Mobility Components.
::
:: $QT_BEGIN_LICENSE:LGPL$
:: GNU Lesser General Public License Usage
:: This file may be used under the terms of the GNU Lesser General Public
:: License version 2.1 as published by the Free Software Foundation and
:: appearing in the file LICENSE.LGPL included in the packaging of this
:: file. Please review the following information to ensure the GNU Lesser
:: General Public License version 2.1 requirements will be met:
:: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
::
:: In addition, as a special exception, Nokia gives you certain additional
:: rights. These rights are described in the Nokia Qt LGPL Exception
:: version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
::
:: GNU General Public License Usage
:: Alternatively, this file may be used under the terms of the GNU General
:: Public License version 3.0 as published by the Free Software Foundation
:: and appearing in the file LICENSE.GPL included in the packaging of this
:: file. Please review the following information to ensure the GNU General
:: Public License version 3.0 requirements will be met:
:: http://www.gnu.org/copyleft/gpl.html.
::
:: Other Usage
:: Alternatively, this file may be used in accordance with the terms and
:: conditions contained in a signed written agreement between you and Nokia.
::
::
::
::
::
:: $QT_END_LICENSE$
::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

@echo off

set QT_MOBILITY_PREFIX= C:\QtMobility
set BUILD_PATH=%CD%
set SOURCE_PATH= %~dp0
cd /D %SOURCE_PATH%
set SOURCE_PATH=%CD%
cd /D %BUILD_PATH%

set PROJECT_CONFIG=%BUILD_PATH%\config.in
set PROJECT_LOG=%BUILD_PATH%\config.log
set RELEASEMODE=release
set WIN32_RELEASEMODE=debug_and_release build_all
set QT_MOBILITY_LIB=
set BUILD_UNITTESTS=no
set BUILD_PUBLIC_UNITTESTS=no
set BUILD_EXAMPLES=no
set BUILD_DEMOS=no
set BUILD_DOCS=yes
set BUILD_TOOLS=yes
set LANGUAGES_CONFIG=
set MOBILITY_MODULES=bearer location contacts systeminfo publishsubscribe versit messaging sensors serviceframework multimedia gallery organizer feedback connectivity
set MOBILITY_MODULES_UNPARSED=
set VC_TEMPLATE_OPTION=
set QT_PATH=
set QMAKE_CACHE=%BUILD_PATH%\.qmake.cache
set PLATFORM_CONFIG=
set NFC_SYMBIAN=auto

REM By default, all modules are requested.  Reset this later if -modules is supplied
set ORGANIZER_REQUESTED=yes

if exist "%QMAKE_CACHE%" del /Q %QMAKE_CACHE%
if exist "%PROJECT_LOG%" del /Q %PROJECT_LOG%
if exist "%PROJECT_CONFIG%" del /Q %PROJECT_CONFIG%

echo QT_MOBILITY_SOURCE_TREE = %SOURCE_PATH:\=/% > %QMAKE_CACHE%
echo QT_MOBILITY_BUILD_TREE = %BUILD_PATH:\=/% >> %QMAKE_CACHE%
set QMAKE_CACHE=

:cmdline_parsing
if "%1" == ""                   goto startProcessing
if "%1" == "-debug"             goto debugTag
if "%1" == "-release"           goto releaseTag
if "%1" == "-silent"            goto silentTag
if "%1" == "-prefix"            goto prefixTag
if "%1" == "-libdir"            goto libTag
if "%1" == "-bindir"            goto binTag
if "%1" == "-headerdir"         goto headerTag
if "%1" == "-plugindir"         goto pluginTag
if "%1" == "-examplesdir"       goto examplesDirTag
if "%1" == "-demosdir"          goto demosDirTag
if "%1" == "-tests"             goto testTag
if "%1" == "-public-tests-only" goto publicTestTag
if "%1" == "-examples"          goto exampleTag
if "%1" == "-demos"             goto demosTag
if "%1" == "-qt"                goto qtTag
if "%1" == "-vc"                goto vcTag
if "%1" == "-no-docs"           goto nodocsTag
if "%1" == "-no-tools"          goto noToolsTag
if "%1" == "-modules"           goto modulesTag
if "%1" == "/?"                 goto usage
if "%1" == "-h"                 goto usage
if "%1" == "-help"              goto usage
if "%1" == "--help"             goto usage
if "%1" == "-symbian-unfrozen"  goto unfrozenTag
if "%1" == "-staticconfig"      goto staticConfigTag
if "%1" == "-languages"         goto languagesTag
if "%1" == "-no-nfc-symbian"    goto noNfcSymbianTag

echo Unknown option: "%1"
goto usage

:usage
echo Usage: configure.bat [-prefix (dir)] [headerdir (dir)] [libdir (dir)]
    echo                  [-bindir (dir)] [-tests] [-examples]
    echo                  [-debug] [-release] [-silent]
    echo.
    echo Options:
    echo.
    echo -prefix (dir) ..... This will install everything relative to dir
    echo                     (default prefix: C:\QtMobility)
    echo -headerdir (dir) .. Header files will be installed to dir
    echo                     (default prefix: PREFIX/include)
    echo -libdir (dir) ..... Libraries will be installed to dir
    echo                     (default PREFIX/lib)
    echo -bindir (dir) ..... Executables will be installed to dir
    echo                     (default PREFIX/bin)
    echo -plugindir (dir) .. Plug-ins will be installed to dir
    echo                     (default PREFIX/plugins)
    echo -examplesdir (dir)  Examples will be installed to dir
    echo                     (default PREFIX/bin)
    echo -demosdir (dir) ... Demos will be installed to dir
    echo                     (default PREFIX/bin)
    echo -debug ............ Build with debugging symbols
    echo -release .......... Build without debugging symbols
    echo -silent ........... Reduces build output
    echo -tests ............ Build unit tests (not build by default)
    echo                     Note, this adds test symbols to all libraries
    echo                     and should not be used for release builds.
    echo -examples ......... Build example applications
    echo -demos ............ Build demo applications
    echo -no-docs .......... Do not build documentation (build by default)
    echo -modules ^<list^> ... Build only the specified modules (default all)
    echo                     Choose from: bearer contacts gallery location publishsubscribe
    echo                     messaging multimedia systeminfo serviceframework
    echo                     sensors versit organizer feedback connectivity
    echo                     Modules should be separated by a space and surrounded
    echo                     by double quotation. If a selected module depends on other modules
    echo                     those modules (and their dependencies) will automatically be enabled.
    echo -vc ............... Generate Visual Studio make files
    echo -languages ........ Languages/translations to be installed (e.g.: ar de ko)
    echo                     (default is empty)
    echo -no-nfc-symbian ... Disables the NFC Symbian backend.


if exist "%PROJECT_CONFIG%" del %PROJECT_CONFIG%
goto exitTag

:qtTag
shift
set QT_PATH=%1\
shift
goto cmdline_parsing

:noNfcSymbianTag
shift
set NFC_SYMBIAN=no
goto cmdline_parsing

:languagesTag
shift
:: %1 can have leading/trailing quotes, so we can't use if "%1" == ""
if xx%1xx == xxxx (
    echo. >&2
    echo >&2The -languages option requires a list of languages.
    echo. >&2
    goto usage
)

:: Remove leading/trailing quotes, if we have them
set MOBILITY_LANGUAGES_UNPARSED=xxx%1xxx
set MOBILITY_LANGUAGES_UNPARSED=%MOBILITY_LANGUAGES_UNPARSED:"xxx=%
set MOBILITY_LANGUAGES_UNPARSED=%MOBILITY_LANGUAGES_UNPARSED:xxx"=%
set MOBILITY_LANGUAGES_UNPARSED=%MOBILITY_LANGUAGES_UNPARSED:xxx=%

REM for now we don't check the languages for their validity
set LANGUAGES_CONFIG=
set LANGUAGES_CONFIG= %MOBILITY_LANGUAGES_UNPARSED%
echo Selected Languages: %LANGUAGES_CONFIG%
shift
goto cmdline_parsing

:staticConfigTag
shift
set PLATFORM_CONFIG=%1
shift
goto cmdline_parsing

:debugTag
if "%RELEASEMODE%" == "release" set RELEASEMODE=debug
set WIN32_RELEASEMODE=
shift
goto cmdline_parsing

:releaseTag
if "%RELEASEMODE%" == "debug"   set RELEASEMODE=release
set WIN32_RELEASEMODE=
shift
goto cmdline_parsing

:silentTag
echo CONFIG += silent > %PROJECT_CONFIG%
shift
goto cmdline_parsing

:prefixTag
shift
set QT_MOBILITY_PREFIX=%1
shift
goto cmdline_parsing

:libTag
shift
echo QT_MOBILITY_LIB = %1 >> %PROJECT_CONFIG%
shift
goto cmdline_parsing

:binTag
shift
echo QT_MOBILITY_BIN = %1 >> %PROJECT_CONFIG%
shift
goto cmdline_parsing

:headerTag
shift
echo QT_MOBILITY_INCLUDE = %1 >> %PROJECT_CONFIG%
shift
goto cmdline_parsing

:pluginTag
shift
echo QT_MOBILITY_PLUGINS = %1 >> %PROJECT_CONFIG%
shift
echo
goto cmdline_parsing

:examplesDirTag
shift
echo QT_MOBILITY_EXAMPLES = %1 >> %PROJECT_CONFIG%
shift
echo
goto cmdline_parsing

:demosDirTag
shift
echo QT_MOBILITY_DEMOS =%1 >> %PROJECT_CONFIG%
shift
echo
goto cmdline_parsing

:unfrozenTag
REM Should never be used in release builds
REM Some SDK's seem to exclude Q_AUTOTEST_EXPORT symbols if the
REM libraries are frozen. This breaks unit tests relying on the auto test exports
REM This flag unfreezes the SYMBIAN libraries for the purpose of unit test building.
REM Ideally this should be connected to '-tests' option but that would prevent
REM integration testing for frozen symbols as the CI system should test unit tests
REM and frozen symbol compliance.
echo symbian_symbols_unfrozen = 1 >> %PROJECT_CONFIG%
shift
goto cmdline_parsing

:testTag
set BUILD_UNITTESTS=yes
shift
goto cmdline_parsing

:publicTestTag
set BUILD_PUBLIC_UNITTESTS=yes
shift
goto cmdline_parsing

:exampleTag
set BUILD_EXAMPLES=yes
shift
goto cmdline_parsing

:demosTag
set BUILD_DEMOS=yes
shift
goto cmdline_parsing

:vcTag
shift
set VC_TEMPLATE_OPTION=-tp vc
goto cmdline_parsing

:nodocsTag
set BUILD_DOCS=no
shift
goto cmdline_parsing

:noToolsTag
set BUILD_TOOLS=no
shift
goto cmdline_parsing

:modulesTag
shift
:: %1 can have leading/trailing quotes, so we can't use if "%1" == ""
if xx%1xx == xxxx (
    echo. >&2
    echo >&2The -modules option requires a list of modules.
    echo. >&2
    goto usage
)

:: Remove leading/trailing quotes, if we have them
set MOBILITY_MODULES_UNPARSED=xxx%1xxx
set MOBILITY_MODULES_UNPARSED=%MOBILITY_MODULES_UNPARSED:"xxx=%
set MOBILITY_MODULES_UNPARSED=%MOBILITY_MODULES_UNPARSED:xxx"=%
set MOBILITY_MODULES_UNPARSED=%MOBILITY_MODULES_UNPARSED:xxx=%

REM reset default modules as we expect a modules list
set MOBILITY_MODULES=
set ORGANIZER_REQUESTED=no

echo Checking selected modules:
:modulesTag2

for /f "tokens=1,*" %%a in ("%MOBILITY_MODULES_UNPARSED%") do (
    set FIRST=%%a
    set REMAINING=%%b
)

: What we want is a switch as we need to check module name and only want to
: distinguish between false and correct module names being passed
if %FIRST% == bearer (
    echo     Bearer Management selected
) else if %FIRST% == contacts (
    echo     Contacts selected
) else if %FIRST% == location (
    echo     Location selected
) else if %FIRST% == messaging (
    echo     Messaging selected
) else if %FIRST% == multimedia (
    echo     Multimedia selected
) else if %FIRST% == publishsubscribe (
    echo     PublishSubscribe selected
) else if %FIRST% == systeminfo (
    echo     Systeminfo selected
) else if %FIRST% == serviceframework (
    echo     ServiceFramework selected
) else if %FIRST% == versit (
    echo     Versit selected ^(implies Contacts^)
) else if %FIRST% == organizer (
    echo     Organizer selected
    set ORGANIZER_REQUESTED=yes
) else if %FIRST% == feedback (
    echo     Feedback selected
) else if %FIRST% == sensors (
    echo     Sensors selected
) else if %FIRST% == gallery (
    echo     Gallery selected
) else if %FIRST% == connectivity (
    echo     Connectivity selected
) else (
    echo     Unknown module %FIRST%
    goto errorTag
)

set MOBILITY_MODULES=%MOBILITY_MODULES% %FIRST%

if "%REMAINING%" == "" (
    shift
) else (
    set MOBILITY_MODULES_UNPARSED=%REMAINING%
    goto modulesTag2
)

SET REMAINING=
SET FIRST=
goto cmdline_parsing

:startProcessing

echo CONFIG += %RELEASEMODE% >> %PROJECT_CONFIG%
echo CONFIG_WIN32 += %WIN32_RELEASEMODE% %RELEASEMODE% >> %PROJECT_CONFIG%
set RELEASEMODE=
set WIN32_RELEASEMODE=

set CURRENTDIR=%CD%
echo %CURRENTDIR%
if exist %QT_MOBILITY_PREFIX% goto prefixExists
mkdir %QT_MOBILITY_PREFIX%
if errorlevel 1 goto invalidPrefix
cd /D %QT_MOBILITY_PREFIX%
set QT_MOBILITY_PREFIX=%CD%
cd /D %CURRENTDIR%
rd /S /Q %QT_MOBILITY_PREFIX%
goto endprefixProcessing

:invalidPrefix
echo "%QT_MOBILITY_PREFIX%" is not a valid directory path.
goto :exitTag

:prefixExists
cd /D %QT_MOBILITY_PREFIX%
set QT_MOBILITY_PREFIX=%CD%
cd /D %CURRENTDIR%

:endprefixProcessing
echo QT_MOBILITY_PREFIX = %QT_MOBILITY_PREFIX:\=/% >> %PROJECT_CONFIG%

echo build_unit_tests = %BUILD_UNITTESTS% >> %PROJECT_CONFIG%
set BUILD_UNITTESTS=

echo build_public_unit_tests = %BUILD_PUBLIC_UNITTESTS% >> %PROJECT_CONFIG%
set BUILD_PUBLIC_UNITTESTS=

echo build_examples = %BUILD_EXAMPLES% >> %PROJECT_CONFIG%
set BUILD_EXAMPLES=

echo build_demos = %BUILD_DEMOS% >> %PROJECT_CONFIG%
set BUILD_DEMOS=

echo build_docs = %BUILD_DOCS% >> %PROJECT_CONFIG%
set BUILD_DOCS=

echo build_tools = %BUILD_TOOLS% >> %PROJECT_CONFIG%
set BUILD_TOOLS=

echo qmf_enabled = no >> %PROJECT_CONFIG%

echo !symbian:isEmpty($$QT_MOBILITY_INCLUDE):QT_MOBILITY_INCLUDE=$$QT_MOBILITY_PREFIX/include >> %PROJECT_CONFIG%
echo isEmpty($$QT_MOBILITY_LIB):QT_MOBILITY_LIB=$$QT_MOBILITY_PREFIX/lib >> %PROJECT_CONFIG%
echo isEmpty($$QT_MOBILITY_BIN):QT_MOBILITY_BIN=$$QT_MOBILITY_PREFIX/bin >> %PROJECT_CONFIG%
echo isEmpty($$QT_MOBILITY_PLUGINS):QT_MOBILITY_PLUGINS=$$QT_MOBILITY_PREFIX/plugins >> %PROJECT_CONFIG%
echo isEmpty($$QT_MOBILITY_EXAMPLES):QT_MOBILITY_EXAMPLES=$$QT_MOBILITY_PREFIX/bin >> %PROJECT_CONFIG%
echo isEmpty($$QT_MOBILITY_DEMOS):QT_MOBILITY_DEMOS=$$QT_MOBILITY_PREFIX/bin >> %PROJECT_CONFIG%

echo selected_languages = %LANGUAGES_CONFIG% >> %PROJECT_CONFIG%

echo mobility_modules = %MOBILITY_MODULES%  >> %PROJECT_CONFIG%
echo contains(mobility_modules,versit): mobility_modules *= contacts >> %PROJECT_CONFIG%
echo contains(mobility_modules,connectivity): mobility_modules *= serviceframework >> %PROJECT_CONFIG%

echo Checking available Qt
call %QT_PATH%qmake -v >> %PROJECT_LOG% 2>&1
if errorlevel 1 goto qmakeNotFound
goto qmakeFound
:qmakeNotFound
echo ... Not found  >> %PROJECT_LOG% 2>&1
if "%QT_PATH%" == "" (
    echo >&2Cannot find 'qmake' in your PATH.
    echo >&2Your PATH is: "%PATH%"
) else (
    echo >&2Cannot find 'qmake' in "%QT_PATH%".
)
echo >&2Aborting.
goto errorTag

:qmakeFound
call %QT_PATH%qmake -query QT_VERSION

goto checkMake

REM Detect make tool
:makeTest
setlocal
    set CURRENT_PWD=%CD%

    if %BUILD_PATH% == %SOURCE_PATH% (
        cd %SOURCE_PATH%\config.tests\make
        if exist make del make
    ) else (
        rmdir /S /Q config.tests\make
        mkdir config.tests\make
        cd config.tests\make
    )

    for /f "tokens=2,3" %%a in ('call %QT_PATH%qmake %SOURCE_PATH%\config.tests\make\make.pro 2^>^&1 1^>NUL') do (
        if "%%a" == "MESSAGE:" (
            set BUILDSYSTEM=%%b)
    )

    if "%BUILDSYSTEM%" == "symbian-abld" (
        call make -h >> %PROJECT_LOG% 2>&1
        if not errorlevel 1 (
            echo ... Symbian abld make found.
            set MAKE=make
        )
    ) else if "%BUILDSYSTEM%" == "symbian-sbsv2" (
        call make -h >> %PROJECT_LOG% 2>&1
        if not errorlevel 1 (
            echo ... Symbian sbsv2 make found.
            set MAKE=make
        )
    ) else if "%BUILDSYSTEM%" == "win32-nmake" (
        call nmake /? >> %PROJECT_LOG% 2>&1
        if not errorlevel 1 (
            echo ... nmake found.
            set MAKE=nmake
        )
    ) else if "%BUILDSYSTEM%" == "win32-mingw" (
        call mingw32-make -v >> %PROJECT_LOG% 2>&1
        if not errorlevel 1 (
            echo ... mingw32-make found.
            set MAKE=mingw32-make
        )
    ) else (
        echo ... Unknown target environment %BUILDSYSTEM%.
    )
    call cd %CURRENT_PWD%
call endlocal&set %1=%MAKE%&set %2=%BUILDSYSTEM%&goto :EOF

:checkMake
echo Checking make
call :makeTest MOBILITY_MAKE MOBILITY_BUILDSYSTEM
if not "%MOBILITY_MAKE%" == "" goto compileTests

echo >&2Cannot find 'nmake', 'mingw32-make' or 'make' in your PATH
echo >&2Aborting.
goto errorTag

:compileTest
setlocal
    @echo off
    echo Checking %1
    set CURRENT_PWD=%CD%

    if %BUILD_PATH% == %SOURCE_PATH% (
        cd %SOURCE_PATH%\config.tests\%2
        if exist %2 del %2
    ) else (
        rmdir /S /Q config.tests\%2
        mkdir config.tests\%2
        cd config.tests\%2
    )

    call %QT_PATH%qmake %SOURCE_PATH%\config.tests\%2\%2.pro >> %PROJECT_LOG% 2>&1

    set FAILED=0
    if "%MOBILITY_BUILDSYSTEM%" == "symbian-sbsv2" (
        call %MOBILITY_MAKE% release-armv5 >> %PROJECT_LOG% 2>&1
        for /f "tokens=2" %%i in ('%MOBILITY_MAKE% release-armv5 SBS^="@sbs --check"') do set FAILED=1
    ) else if "%MOBILITY_BUILDSYSTEM%" == "symbian-abld" (
        call %MOBILITY_MAKE% release-gcce >> %PROJECT_LOG% 2>&1
        for /f "tokens=2" %%i in ('%MOBILITY_MAKE% release-gcce ABLD^="@ABLD.BAT -c" 2^>^&1') do if not %%i == bldfiles set FAILED=1
    ) else (
        REM Make for other builds
        call %MOBILITY_MAKE% >> %PROJECT_LOG% 2>&1
        REM have to check error level for windows / other builds to be sure.
        if errorlevel 1 (
           set FAILED=1
        )
    )

    if %FAILED% == 0 (
        echo ... OK
        echo %2_enabled = yes >> %PROJECT_CONFIG%
    ) else (
        echo ... Not Found
        echo %2_enabled = no >> %PROJECT_CONFIG%
    )

    cd %CURRENT_PWD%
endlocal&goto :EOF

:compileTests

REM No reason to do config tests if we got a platform configuration
if not "%PLATFORM_CONFIG%" == "" goto platformconfig

REM We shouldn't enable some of these if the corresponding modules are not enabled
echo.
echo Start of compile tests
REM compile tests go here.
for /f "tokens=3" %%i in ('call %QT_PATH%qmake %SOURCE_PATH%\config.tests\make\make.pro 2^>^&1 1^>NUL') do set BUILDSYSTEM=%%i
if "%BUILDSYSTEM%" == "symbian-abld" goto symbianTests
if "%BUILDSYSTEM%" == "symbian-sbsv2" (
  perl -S  %SOURCE_PATH%\bin\compilercheck.pl
  goto symbianTests
)
goto windowsTests

:symbianTests

call :compileTest LBT lbt
call :compileTest SNAP snap
call :compileTest OCC occ
call :compileTest SymbianENote symbianenote
call :compileTest SymbianContactSIM symbiancntsim
call :compileTest SymbianContactModel symbiancntmodel
call :compileTest SymbianContactModelv2 symbiancntmodelv2
call :compileTest S60_Sensor_API sensors_s60_31
call :compileTest Symbian_Sensor_Framework sensors_symbian
call :compileTest Sensor_Framework_Light sensors_symbian_light
call :compileTest Symbian_Hb hb_symbian
call :compileTest Audiorouting_s60 audiorouting_s60
call :compileTest Tunerlibrary_for_3.1 tunerlib_s60
call :compileTest RadioUtility_for_post_3.1 radioutility_s60
REM Currently not being supported
REM call :compileTest OpenMaxAl_support openmaxal_symbian
call :compileTest Surfaces_s60 surfaces_s60
call :compileTest Symbian_Messaging_Freestyle messaging_freestyle
call :compileTest Symbian_Messaging_Freestyle_MAPI12 messaging_freestyle_mapi12
call :compileTest callinformation_symbian callinformation_symbian
call :compileTest IMMERSION immersion
call :compileTest Symbian_CamAutoFocus_31 symbian_camera_camautofocus
call :compileTest Symbian_ECamAdvancedSettings_Post31 symbian_camera_ecamadvsettings
call :compileTest Symbian_DevVideoRecord symbian_camera_devvideorecord
call :compileTest Symbian_SnapShotAPI symbian_camera_snapshot
call :compileTest AdvancedTouchFeedback advancedtouchfeedback
call :compileTest CHWRMHaptics chwrmhaptics
call :compileTest MDS mds
call :compileTest MDS_25 mds_25
call :compileTest MDS_25_92MCL mds_25_92mcl
if "%NFC_SYMBIAN%" == "auto" (
    call :compileTest Symbian_NFC nfc_symbian
) else (
    echo nfc_symbian_enabled = no >> %PROJECT_CONFIG%
)
call :compileTest BTEngineConnectionManager_Symbian btengconnman_symbian
call :compileTest BTEngineDeviceManager_Symbian btengdevman_symbian
call :compileTest LockandFlipKeys LockandFlipPSkeys
call :compileTest FmTxClientCheck FmTxClient
call :compileTest DiskNotifyClientCheck DiskNotifyClient
call :compileTest ThermalStatus_Symbian thermalstatus_symbian
call :compileTest Flextimer_Symbian symbianflextimer
call :compileTest ETelPacketservice_symbian etelpacketservice_symbian
call :compileTest mmf_http_cookies mmf_http_cookies
goto noTests

:windowsTests

call :compileTest DirectShow directshow
call :compileTest WindowsMediaSDK wmsdk
call :compileTest WindowMediaPlayer wmp
call :compileTest EnhancedVideoRenderer evr

:noTests

echo End of compile tests
echo.
echo.
goto processHeaders

:platformconfig

echo.
echo Skipping configure tests
echo Loading ... features\platformconfig\%PLATFORM_CONFIG%.pri

if not exist "%SOURCE_PATH%\features\platformconfig\%PLATFORM_CONFIG%.pri" (
    echo >&2Invalid platform configuration %PLATFORM_CONFIG%.pri
    goto errorTag
)
echo include($${QT_MOBILITY_SOURCE_TREE}/features/platformconfig/%PLATFORM_CONFIG%.pri) >> %PROJECT_CONFIG%

:processHeaders
REM we could skip generating headers if a module is not enabled
if not exist "%BUILD_PATH%\features" mkdir %BUILD_PATH%\features
if not exist "%BUILD_PATH%\features\strict_flags.prf" copy %SOURCE_PATH%\features\strict_flags.prf %BUILD_PATH%\features

echo Generating Mobility Headers...
rd /s /q %BUILD_PATH%\include
mkdir %BUILD_PATH%\include
perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include %SOURCE_PATH%\src\global

set MODULES_TEMP=%MOBILITY_MODULES%

:generateHeaders

for /f "tokens=1,*" %%a in ("%MODULES_TEMP%") do (
    set FIRST=%%a
    set REMAINING=%%b
)

if %FIRST% == bearer (
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtBearer %SOURCE_PATH%\src\bearer
) else if %FIRST% == contacts (
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtContacts %SOURCE_PATH%\src\contacts
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtContacts %SOURCE_PATH%\src\contacts\requests
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtContacts %SOURCE_PATH%\src\contacts\filters
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtContacts %SOURCE_PATH%\src\contacts\details
) else if %FIRST% == location (
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtLocation %SOURCE_PATH%\src\location
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtLocation %SOURCE_PATH%\src\location\landmarks
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtLocation %SOURCE_PATH%\src\location\maps
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtLocation %SOURCE_PATH%\src\location\maps\tiled
) else if %FIRST% == messaging (
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtMessaging %SOURCE_PATH%\src\messaging
) else if %FIRST% == multimedia (
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtMultimediaKit %SOURCE_PATH%\src\multimedia
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtMultimediaKit %SOURCE_PATH%\src\multimedia\audio
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtMultimediaKit %SOURCE_PATH%\src\multimedia\video
) else if %FIRST% == publishsubscribe (
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtPublishSubscribe %SOURCE_PATH%\src\publishsubscribe
) else if %FIRST% == systeminfo (
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtSystemInfo %SOURCE_PATH%\src\systeminfo
) else if %FIRST% == serviceframework (
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtServiceFramework %SOURCE_PATH%\src\serviceframework
) else if %FIRST% == versit (
    REM versit implies contacts.  organizer includes might also be necessary
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtVersit %SOURCE_PATH%\src\versit
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtContacts %SOURCE_PATH%\src\contacts
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtContacts %SOURCE_PATH%\src\contacts\requests
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtContacts %SOURCE_PATH%\src\contacts\filters
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtContacts %SOURCE_PATH%\src\contacts\details
    if %ORGANIZER_REQUESTED% == yes (
        perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtVersitOrganizer %SOURCE_PATH%\src\versitorganizer
    )
) else if %FIRST% == sensors (
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtSensors %SOURCE_PATH%\src\sensors
) else if %FIRST% == gallery (
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtGallery %SOURCE_PATH%\src\gallery
) else if %FIRST% == organizer (
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtOrganizer %SOURCE_PATH%\src\organizer
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtOrganizer %SOURCE_PATH%\src\organizer\items
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtOrganizer %SOURCE_PATH%\src\organizer\requests
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtOrganizer %SOURCE_PATH%\src\organizer\filters
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtOrganizer %SOURCE_PATH%\src\organizer\details
) else if %FIRST% == feedback (
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtFeedback %SOURCE_PATH%\src\feedback
) else if %FIRST% == connectivity (
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtConnectivity %SOURCE_PATH%\src\connectivity
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtConnectivity %SOURCE_PATH%\src\connectivity\nfc
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtConnectivity %SOURCE_PATH%\src\connectivity\bluetooth
    REM connectivity/nfc implies sfw
    perl -S %SOURCE_PATH%\bin\syncheaders %BUILD_PATH%\include\QtServiceFramework %SOURCE_PATH%\src\serviceframework
)

if "%REMAINING%" == "" (
    shift
) else (
    set MODULES_TEMP=%REMAINING%
    goto generateHeaders
)

SET REMAINING=
SET FIRST=
SET MODULES_TEMP=

if exist config.pri del config.pri
ren %PROJECT_CONFIG% config.pri

echo.
echo Running qmake...
call %QT_PATH%qmake -recursive %VC_TEMPLATE_OPTION% %SOURCE_PATH%\qtmultimediakit.pro
if errorlevel 1 goto qmakeRecError
echo.
echo configure has finished. You may run %MOBILITY_MAKE% to build the project now.
goto exitTag

:qmakeRecError
echo.
echo configure failed.
goto errorTag

REM Unset the internal variables
:errorTag
set BUILD_PATH=
set CURRENTDIR=
set MOBILITY_MAKE=
set MOBILITY_BUILDSYSTEM=
set PROJECT_CONFIG=
set PROJECT_LOG=
set QT_MOBILITY_PREFIX=
set QT_PATH=
set SOURCE_PATH=
set MOBILITY_MODULES=
set MOBILITY_MODULES_UNPARSED=
set REMAINING=
set FIRST=
set MODULES_TEMP=
set QT_MOBILITY_EXAMPLES=
set QT_MOBILITY_DEMOS=
set ORGANIZER_REQUESTED=
set PLATFORM_CONFIG=
set LANGUAGES_CONFIG=
exit /b 1

:exitTag
set BUILD_PATH=
set CURRENTDIR=
set MOBILITY_MAKE=
set MOBILITY_BUILDSYSTEM=
set PROJECT_CONFIG=
set PROJECT_LOG=
set QT_MOBILITY_PREFIX=
set QT_PATH=
set SOURCE_PATH=
set MOBILITY_MODULES=
set MOBILITY_MODULES_UNPARSED=
set REMAINING=
set FIRST=
set MODULES_TEMP=
set QT_MOBILITY_EXAMPLES=
set QT_MOBILITY_DEMOS=
set ORGANIZER_REQUESTED=
set PLATFORM_CONFIG=
set LANGUAGES_CONFIG=
exit /b 0
