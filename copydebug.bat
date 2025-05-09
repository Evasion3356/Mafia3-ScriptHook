@echo off
set MAFIAPATH=D:\SteamLibrary\steamapps\common\Mafia III
set mypath=%~dp0

copy /Y /B "%mypath%\build\Debug\dxgi.dll" "%MAFIAPATH%\dxgi.dll" /B 
copy /Y /B "%mypath%\build\Debug\dxgi.pdb" "%MAFIAPATH%\dxgi.pdb" /B 