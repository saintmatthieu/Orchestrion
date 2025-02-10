!define APPNAME "MyApp"
!define VERSION "1.0"
Outfile "build\install\Install Orchestrion.exe"
InstallDir "$PROGRAMFILES64\saintmatthieu"

Section "Install"
    SetOutPath $INSTDIR\bin
    File /r "build\install\bin\*.*"
    SetOutPath $INSTDIR\include
    File /r "build\install\include\*.*"
    SetOutPath $INSTDIR\lib
    File /r "build\install\lib\*.*"
    SetOutPath $INSTDIR\scores
    File /r "build\install\scores\*.*"
    SetOutPath $INSTDIR\sound
    File /r "build\install\sound\*.*"
    SetOutPath $INSTDIR\wallpapers
    File /r "build\install\wallpapers\*.*"
    CreateShortcut "$DESKTOP\Orchestrion.lnk" "$INSTDIR\bin\OrchestrionApp.exe"
SectionEnd
