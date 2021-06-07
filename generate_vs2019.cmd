mkdir build\vs2019_x86
pushd build\vs2019_x86
cmake -G "Visual Studio 16 2019" -A Win32 %* ../..
popd
pause