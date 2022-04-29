@echo off

set opts=-FC -GR- -EHa- -nologo -Zi
set code=%cd%
if not exist build mkdir build
pushd build
cl %opts% %code%\code\main.c -Fesergen.exe
popd
