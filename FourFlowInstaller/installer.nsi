; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

; The name of the installer
Name "FourFlow"

; The file to write
OutFile "FourFlowInstaller.exe"

; The default installation directory
InstallDir $PROGRAMFILES\FourFlow

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\FourFlow" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "Components (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "ParaView Users Guide v3.14.pdf"
  File "FourFlowDocumentation.pdf"
  File /r "bin"
  File /r "lib"
  File /r "include"
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\NSIS_Example2 "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FourFlow" "DisplayName" "FourFlow"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FourFlow" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FourFlow" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FourFlow" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\FourFlow"
  CreateShortCut "$SMPROGRAMS\FourFlow\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\FourFlow\ParaView Users Guide v3.14.lnk" "$INSTDIR\ParaView Users Guide v3.14.pdf" "" "$INSTDIR\ParaView Users Guide v3.14.pdf" 0
  CreateShortCut "$SMPROGRAMS\FourFlow\Four Flow Documentation.lnk" "$INSTDIR\FourFlowDocumentation.pdf" "" "$INSTDIR\FourFlowDocumentation.pdf" 0
  CreateShortCut "$SMPROGRAMS\FourFlow\FourFlow.lnk" "$INSTDIR\bin\FourFlow.exe" "" "$INSTDIR\bin\FourFlow.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\FourFlow"
  DeleteRegKey HKLM SOFTWARE\FourFlow

  ; Remove files and uninstaller
  Delete "$INSTDIR\ParaView Users Guide v3.14"
  Delete $INSTDIR\uninstall.exe

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\FourFlow\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\FourFlow"
  RMDir "$INSTDIR"

SectionEnd
