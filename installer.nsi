!define APPNAME "MyApp"
!define VERSION "1.0"
Outfile "build\InstallOrchestrion.exe"
InstallDir "$PROGRAMFILES64\saintmatthieu"

Section "Install"
    SetOutPath $INSTDIR\bin
    File "build\install\bin\OrchestrionApp.exe"
    File "build\install\bin\libsndfile-1.dll"
    SetOutPath $INSTDIR\scores
    File /r "build\install\scores\*.*"
    SetOutPath $INSTDIR\sound
    File /r "build\install\sound\*.*"
    SetOutPath $INSTDIR\wallpapers
    File /r "build\install\wallpapers\*.*"
    CreateShortcut "$DESKTOP\Orchestrion.lnk" "$INSTDIR\bin\OrchestrionApp.exe"
SectionEnd
