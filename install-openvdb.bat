cd openvdb
if not exist build mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX="../../OpenVDBInstall" -A x64 ..
cmake --build . -j --config Debug --target install
cmake --build . -j --config Release --target install
