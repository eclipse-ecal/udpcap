mkdir _build
cd _build

cmake .. -DCMAKE_INSTALL_PREFIX=_install -A x64 -DBUILD_SHARED_LIBS=OFF
cd ..
pause
