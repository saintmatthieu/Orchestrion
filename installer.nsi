!define APPNAME "MyApp"
!define VERSION "1.0"
Outfile "build\Orchestrion installer for Windows.exe"
InstallDir "$PROGRAMFILES64\saintmatthieu\Orchestrion"

; Define the icon for the installer and shortcut
Icon "build\install\icons\music-box.ico"

Section "Install"

    SetOutPath $INSTDIR\bin
    File "build\install\bin\Orchestrion.exe"
    File "build\install\bin\libsndfile-1.dll"
    File "${QtInstallDir}\bin\Qt6Core.dll"
    File "${QtInstallDir}\bin\Qt6Core5Compat.dll"
    File "${QtInstallDir}\bin\Qt6Gui.dll"
    File "${QtInstallDir}\bin\Qt6Network.dll"
    File "${QtInstallDir}\bin\Qt6OpenGL.dll"
    File "${QtInstallDir}\bin\Qt6Qml.dll"
    File "${QtInstallDir}\bin\Qt6QmlModels.dll"
    File "${QtInstallDir}\bin\Qt6QmlWorkerScript.dll"
    File "${QtInstallDir}\bin\Qt6Quick.dll"
    File "${QtInstallDir}\bin\Qt6QuickControls2.dll"
    File "${QtInstallDir}\bin\Qt6QuickControls2Impl.dll"
    File "${QtInstallDir}\bin\Qt6QuickLayouts.dll"
    File "${QtInstallDir}\bin\Qt6QuickTemplates2.dll"
    File "${QtInstallDir}\bin\Qt6QuickWidgets.dll"
    File "${QtInstallDir}\bin\Qt6StateMachine.dll"
    File "${QtInstallDir}\bin\Qt6Svg.dll"
    File "${QtInstallDir}\bin\Qt6Widgets.dll"

    SetOutPath $INSTDIR\bin\imageformats
    File "${QtInstallDir}\plugins\imageformats\qsvg.dll"
    File "${QtInstallDir}\plugins\imageformats\qjpeg.dll"

    SetOutPath $INSTDIR\bin\platforms
    File "${QtInstallDir}\plugins\platforms\qwindows.dll"

    SetOutPath $INSTDIR\qml\Qt5Compat
    File /r "${QtInstallDir}\qml\Qt5Compat\*.*"
    SetOutPath $INSTDIR\qml\QtCore
    File /r "${QtInstallDir}\qml\QtCore\*.*"
    SetOutPath $INSTDIR\qml\QtQml
    File /r "${QtInstallDir}\qml\QtQml\*.*"
    SetOutPath $INSTDIR\qml\QtQuick
    File /r "${QtInstallDir}\qml\QtQuick\*.*"

    SetOutPath $INSTDIR\scores
    File /r "build\install\scores\*.*"

    SetOutPath $INSTDIR\sound
    File /r "build\install\sound\*.*"

    SetOutPath $INSTDIR\wallpapers
    File /r "build\install\wallpapers\*.*"

    SetOutPath $INSTDIR\icons
    File /r "build\install\icons\*.*"

    SetOutPath $INSTDIR\icons\controllers
    File /r "build\install\icons\controllers\*.*"

    ; Create a shortcut with the specified icon
    CreateShortcut "$DESKTOP\Orchestrion.lnk" "$INSTDIR\bin\Orchestrion.exe" "" "$INSTDIR\icons\music-box.ico"
SectionEnd
