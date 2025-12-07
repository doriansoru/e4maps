Name "e4maps"
OutFile "e4maps-installer.exe"
InstallDir $PROGRAMFILES\e4maps
InstallDirRegKey HKCU "Software\e4maps" ""

Section
  SetOutPath $INSTDIR
  File /r dist\*.*

  ; Create start menu and desktop shortcuts
  CreateDirectory "$SMPROGRAMS\e4maps"
  CreateShortCut "$SMPROGRAMS\e4maps\e4maps.lnk" "$INSTDIR\e4maps.exe" "" "$INSTDIR\e4maps.exe" 0 SW_SHOWNORMAL
  CreateShortCut "$DESKTOP\e4maps.lnk" "$INSTDIR\e4maps.exe" "" "$INSTDIR\e4maps.exe" 0 SW_SHOWNORMAL

  WriteRegStr HKCU "Software\e4maps" "" $INSTDIR
  WriteRegStr HKCR "Software\Microsoft\Windows\CurrentVersion\Uninstall\e4maps" "DisplayName" "e4maps"
  WriteRegStr HKCR "Software\Microsoft\Windows\CurrentVersion\Uninstall\e4maps" "UninstallString" "$INSTDIR\uninstall.exe"

  ; Create uninstall shortcut in start menu
  CreateShortCut "$SMPROGRAMS\e4maps\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0 SW_SHOWNORMAL

  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
  ; Remove start menu and desktop shortcuts
  Delete "$SMPROGRAMS\e4maps\e4maps.lnk"
  RMDir "$SMPROGRAMS\e4maps"
  Delete "$DESKTOP\e4maps.lnk"

  DeleteRegKey HKCU "Software\e4maps"
  DeleteRegKey HKCR "Software\Microsoft\Windows\CurrentVersion\Uninstall\e4maps"
  RMDir /r "$INSTDIR"
SectionEnd
