@echo off
REM make sure 'wasm' folder of the WebAssembly build environment is placed in the parent folder of this script
REM it can be downloaded from https://github.com/mattiasgustavsson/dos-like/releases/tag/wasm-env
REM also, the yarnspin.exe needs to be in the parent folder of this script, so be sure to run build_win.bat first
pushd %~dp0
pushd ..
yarnspin --compile
popd
..\wasm\node ..\wasm\wajicup.js -embed yarnspin.dat ../yarnspin.dat -template template.html -rle ../source/yarnspin.c yarnspin.html
popd