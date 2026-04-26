; How to run this installer script:
;   makensis installer.nsi
;     - Auto-detects BUILD_INSTALL_DIR in this order:
;       build\install, build\Release\install, build\RelWithDebInfo\install, build\Debug\install.
;   makensis /DBUILD_CONFIG=Release installer.nsi
;     - Uses build\Release\install.
;   makensis /DBUILD_INSTALL_DIR="build\Release\install" installer.nsi
;     - Uses an explicit install tree path.

!define APPNAME "MyApp"
!define VERSION "1.0"
!ifndef BUILD_CONFIG
!if /FileExists "build\Release\install\bin\Orchestrion.exe"
!define BUILD_CONFIG "Release"
!else
!if /FileExists "build\RelWithDebInfo\install\bin\Orchestrion.exe"
!define BUILD_CONFIG "RelWithDebInfo"
!else
!define BUILD_CONFIG "Debug"
!endif
!endif
!endif

!ifndef BUILD_INSTALL_DIR
!if /FileExists "build\install\bin\Orchestrion.exe"
!define BUILD_INSTALL_DIR "build\install"
!else
!define BUILD_INSTALL_DIR "build\${BUILD_CONFIG}\install"
!endif
!endif

!if /FileExists "${BUILD_INSTALL_DIR}\bin\Orchestrion.exe"
!else
!error "BUILD_INSTALL_DIR ('${BUILD_INSTALL_DIR}') does not exist or is missing Orchestrion.exe. Run 'cmake --install' for the chosen configuration, or pass /DBUILD_INSTALL_DIR=... to makensis."
!endif
Outfile "build\Orchestrion installer for Windows.exe"
InstallDir "$PROGRAMFILES64\saintmatthieu\Orchestrion"

; Define the icon for the installer and shortcut
Icon "icons\music-box.ico"

Section "Install"

    SetOutPath $INSTDIR\bin
    File /r "${BUILD_INSTALL_DIR}\bin\*.*"

    SetOutPath $INSTDIR\qml
    File /r "${BUILD_INSTALL_DIR}\qml\*.*"

    SetOutPath $INSTDIR\plugins
    File /r "${BUILD_INSTALL_DIR}\plugins\*.*"

    SetOutPath $INSTDIR\scores
    File /r "${BUILD_INSTALL_DIR}\scores\*.*"

    SetOutPath $INSTDIR\sound
    File /r "${BUILD_INSTALL_DIR}\sound\*.*"

    SetOutPath $INSTDIR\instruments
    File /r "${BUILD_INSTALL_DIR}\instruments\*.*"

    SetOutPath $INSTDIR\wallpapers
    File /r "${BUILD_INSTALL_DIR}\wallpapers\*.*"

    SetOutPath $INSTDIR\icons
    File /r "${BUILD_INSTALL_DIR}\icons\*.*"

    SetOutPath $INSTDIR\icons\controllers
    File /r "${BUILD_INSTALL_DIR}\icons\controllers\*.*"

    SetOutPath $INSTDIR\locale
    File /r "${BUILD_INSTALL_DIR}\locale\*.*"

    SetOutPath $INSTDIR\translations
    File /r "${BUILD_INSTALL_DIR}\translations\*.*"

    ; Create a shortcut with the specified icon
    CreateShortcut "$DESKTOP\Orchestrion.lnk" "$INSTDIR\bin\Orchestrion.exe" "" "$INSTDIR\icons\music-box.ico"
SectionEnd
