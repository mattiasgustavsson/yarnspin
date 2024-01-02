@echo off
where /q cl
if ERRORLEVEL 1 (
	echo Run this from a MSVC Developer Command Prompt
	pause
	goto :eof
)
pushd %~dp0
rc /nologo yarnspin.rc
cl ..\source\yarnspin.c yarnspin.res /nologo /O2 /Ob2 /Oi /Ot /Oy /MT /GL /GF /D "NDEBUG" /Fe:..\yarnspin.exe /link /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG /SUBSYSTEM:CONSOLE 
cl ..\source\yarnspin.c yarnspin.res /nologo /O2 /Ob2 /Oi /Ot /Oy /MT /GL /GF /D "NDEBUG" /D "YARNSPIN_RUNTIME_ONLY" /Fe:runtime.exe /link /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG /SUBSYSTEM:WINDOWS
del yarnspin.res
del yarnspin.obj
if EXIST ..\wasm\node.exe (
	..\wasm\node ..\wasm\wajicup.js ../source/yarnspin.c runtime.wasm
    ..\wasm\node ..\wasm\wajicup.js -template template.html runtime.wasm runtime.html -no_minify
	popd
	goto :eof
)
echo To build wasm runtime, make sure the 'wasm' folder of the WebAssembly build environment is placed in the parent folder of this script (in the yarnspin root folder).
echo It can be downloaded from https://github.com/mattiasgustavsson/dos-like/releases/tag/wasm-env
echo Skipping build for wasm runtime.
popd