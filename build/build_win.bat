@echo off
REM run this from a MSVC Developer Command Prompt
pushd %~dp0
rc /nologo yarnspin.rc
cl ..\source\yarnspin.c yarnspin.res /nologo /O2 /Ob2 /Oi /Ot /Oy /MT /GL /GF /D "NDEBUG" /Fe:..\yarnspin.exe /link /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG /SUBSYSTEM:WINDOWS 
pushd ..
yarnspin --compile
popd
cl ..\source\yarnspin.c yarnspin.res /nologo /O2 /Ob2 /Oi /Ot /Oy /MT /GL /GF /D "NDEBUG" /D "YARNSPIN_RUNTIME_ONLY" /Fe:runtime.exe /link /INCREMENTAL:NO /OPT:REF /OPT:ICF /LTCG /SUBSYSTEM:WINDOWS 
copy /b runtime.exe + ..\yarnspin.dat ..\game.exe
popd
