@echo off

set opts=-FC -GR- -EHa- -nologo -Zi
set code=%cd%
pushd build
cl %opts% %code%\example\test_main.cpp -Fesergen_test.exe
popd
