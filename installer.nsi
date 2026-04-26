!define APPNAME "MyApp"
!define VERSION "1.0"
Outfile "build\Orchestrion installer for Windows.exe"
InstallDir "$PROGRAMFILES64\saintmatthieu\Orchestrion"

; Define the icon for the installer and shortcut
Icon "build\install\icons\music-box.ico"

Section "Install"

    SetOutPath $INSTDIR\bin
    File /r "build\install\bin\*.*"

    SetOutPath $INSTDIR\qml
    File /r "build\install\qml\*.*"

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

    SetOutPath $INSTDIR\locale
    File /r "build\install\locale\*.*"

    ; Create a shortcut with the specified icon
    CreateShortcut "$DESKTOP\Orchestrion.lnk" "$INSTDIR\bin\Orchestrion.exe" "" "$INSTDIR\icons\music-box.ico"
SectionEnd
