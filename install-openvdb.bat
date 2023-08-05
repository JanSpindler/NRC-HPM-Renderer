cd openvdb
if not exist build mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH="../../openvdb-install" -A x64 ..
cmake --build . --parallel 16 --config Release --target install
