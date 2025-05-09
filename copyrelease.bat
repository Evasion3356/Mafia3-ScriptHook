@echo off
set MAFIAPATH=D:\SteamLibrary\steamapps\common\Mafia III
set mypath=%~dp0

copy /Y /B "%mypath%\build\Release\dxgi.dll" "%MAFIAPATH%\dxgi.dll" /B 
del /F /Q "%MAFIAPATH%\dxgi.pdb"