# libtguy

## Build instructions
```sh
git clone https://github.com/Wirtos/libtguy
cd libtguy
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -t install
```
- To build doxygen documentation, add `-DTGUY_BUILD_DOCS=ON` when configuring
- To build libtguy as a shared library, add `-DBUILD_SHARED_LIBS=ON`
- Libraries, headers and cmake config files will be installed to `build/install`
- For advanced manual configuration process and list of more advance options use ccmake or CMake-GUI isntead of cmake
