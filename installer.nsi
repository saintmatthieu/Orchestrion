!define APPNAME "MyApp"
!define VERSION "1.0"
Outfile "build\Install Orchestrion.exe"
InstallDir "$PROGRAMFILES64\saintmatthieu"

; Define the icon for the installer and shortcut
Icon "build\install\icons\music-box.ico"

Section "Install"
    SetOutPath $INSTDIR\bin
    File "build\install\bin\Orchestrion.exe"
    File "build\install\bin\libsndfile-1.dll"

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
