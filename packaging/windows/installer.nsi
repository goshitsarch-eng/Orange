Unicode true
!include "MUI2.nsh"
Name "Orange Music Player"
OutFile "${OUTPUT}"
InstallDir "$LOCALAPPDATA\Programs\Orange"
RequestExecutionLevel user
Icon "..\..\desktop\windows\runner\resources\app_icon.ico"
UninstallIcon "..\..\desktop\windows\runner\resources\app_icon.ico"
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\COPYING"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
Section "Orange"
  SetOutPath "$INSTDIR"
  File /r "${STAGE}\*"
  CreateDirectory "$SMPROGRAMS\Orange"
  CreateShortcut "$SMPROGRAMS\Orange\Orange.lnk" "$INSTDIR\orange.exe"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateShortcut "$SMPROGRAMS\Orange\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Orange" "DisplayName" "Orange Music Player ${VERSION}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Orange" "UninstallString" '$"$INSTDIR\Uninstall.exe$"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Orange" "DisplayIcon" "$INSTDIR\orange.exe"
  WriteRegStr HKCU "Software\Classes\Applications\orange.exe" "FriendlyAppName" "Orange Music Player"
  WriteRegStr HKCU "Software\Classes\Applications\orange.exe\shell\open\command" "" '$"$INSTDIR\orange.exe$" $"%1$"'
  WriteRegStr HKCU "Software\Classes\com.goshapps.Orange.Audio" "" "Orange audio file"
  WriteRegStr HKCU "Software\Classes\com.goshapps.Orange.Audio\DefaultIcon" "" "$INSTDIR\orange.exe,0"
  WriteRegStr HKCU "Software\Classes\com.goshapps.Orange.Audio\shell\open\command" "" '$"$INSTDIR\orange.exe$" $"%1$"'
  !macro register_extension extension
    WriteRegStr HKCU "Software\Classes\${extension}\OpenWithProgids" "com.goshapps.Orange.Audio" ""
  !macroend
  !insertmacro register_extension ".flac"
  !insertmacro register_extension ".mp3"
  !insertmacro register_extension ".wav"
  !insertmacro register_extension ".m3u8"
  !insertmacro register_extension ".m3u"
  !insertmacro register_extension ".pls"
  !insertmacro register_extension ".xspf"
SectionEnd
Section "Uninstall"
  !macro unregister_extension extension
    DeleteRegValue HKCU "Software\Classes\${extension}\OpenWithProgids" "com.goshapps.Orange.Audio"
  !macroend
  !insertmacro unregister_extension ".flac"
  !insertmacro unregister_extension ".mp3"
  !insertmacro unregister_extension ".wav"
  !insertmacro unregister_extension ".m3u8"
  !insertmacro unregister_extension ".m3u"
  !insertmacro unregister_extension ".pls"
  !insertmacro unregister_extension ".xspf"
  DeleteRegKey HKCU "Software\Classes\com.goshapps.Orange.Audio"
  DeleteRegKey HKCU "Software\Classes\Applications\orange.exe"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Orange"
  RMDir /r "$SMPROGRAMS\Orange"
  RMDir /r "$INSTDIR"
  ; Per-user settings and collection data are intentionally retained.
SectionEnd
