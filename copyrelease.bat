@echo off
set MAFIAPATH=C:\Program Files (x86)\Steam\steamapps\common\Mafia III
set mypath=%~dp0

copy /Y /B "%mypath%\build\Release\dxgi.dll" "%MAFIAPATH%\dxgi.dll" /B 
del /F /Q "%MAFIAPATH%\dxgi.pdb"