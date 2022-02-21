mkdir build\vs2022_x86
pushd build\vs2022_x86
cmake -G "Visual Studio 17 2022" -A Win32 %* ../..
popd
pause