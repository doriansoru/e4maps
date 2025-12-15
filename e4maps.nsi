Name "e4maps"
OutFile "e4maps-installer.exe"
InstallDir $PROGRAMFILES\e4maps
InstallDirRegKey HKCU "Software\e4maps" ""

!include "MUI2.nsh"

; Installer pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"  ; If you have a license file
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Language
!insertmacro MUI_LANGUAGE "English"

; Define sections for optional components
Section "Main Application" SecMain
  SetOutPath $INSTDIR
  File /r dist\*.*

  ; Create start menu shortcuts
  CreateDirectory "$SMPROGRAMS\e4maps"
  CreateShortCut "$SMPROGRAMS\e4maps\e4maps.lnk" "$INSTDIR\e4maps.exe" "" "$INSTDIR\e4maps.exe" 0 SW_SHOWNORMAL

  WriteRegStr HKCU "Software\e4maps" "" $INSTDIR
  WriteRegStr HKCR "Software\Microsoft\Windows\CurrentVersion\Uninstall\e4maps" "DisplayName" "e4maps"
  WriteRegStr HKCR "Software\Microsoft\Windows\CurrentVersion\Uninstall\e4maps" "UninstallString" "$INSTDIR\uninstall.exe"

  ; Create uninstall shortcut in start menu
  CreateShortCut "$SMPROGRAMS\e4maps\Uninstall.lnk" "$INSTDIR\Uninstall.exe" "" "$INSTDIR\Uninstall.exe" 0 SW_SHOWNORMAL

  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

; Optional Desktop Shortcut
Section "Desktop Shortcut" SecDesktop
  CreateShortCut "$DESKTOP\e4maps.lnk" "$INSTDIR\e4maps.exe" "" "$INSTDIR\e4maps.exe" 0 SW_SHOWNORMAL
SectionEnd

; Optional Start Menu Entry
SectionGroup /e "Start Menu Options"
  Section "Start Menu Folder" SecStartMenu
    CreateDirectory "$SMPROGRAMS\e4maps"
    CreateShortCut "$SMPROGRAMS\e4maps\e4maps.lnk" "$INSTDIR\e4maps.exe" "" "$INSTDIR\e4maps.exe" 0 SW_SHOWNORMAL
  SectionEnd
SectionGroupEnd

; Uninstaller Section
Section "Uninstall"
  ; Remove start menu and desktop shortcuts
  Delete "$SMPROGRAMS\e4maps\e4maps.lnk"
  RMDir "$SMPROGRAMS\e4maps"
  Delete "$DESKTOP\e4maps.lnk"

  DeleteRegKey HKCU "Software\e4maps"
  DeleteRegKey HKCR "Software\Microsoft\Windows\CurrentVersion\Uninstall\e4maps"
  RMDir /r "$INSTDIR"
SectionEnd

LangString DESC_SecMain ${LANG_ENGLISH} "Main application files."
LangString DESC_SecDesktop ${LANG_ENGLISH} "Place a shortcut on the desktop."
LangString DESC_SecStartMenu ${LANG_ENGLISH} "Create start menu entry."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(DESC_SecMain)
!insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} $(DESC_SecDesktop)
!insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenu} $(DESC_SecStartMenu)
!insertmacro MUI_FUNCTION_DESCRIPTION_END
