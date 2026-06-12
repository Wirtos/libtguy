# libtguy

## Build instructions
```sh
git clone --recursive https://github.com/Wirtos/libtguy
cd libtguy
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=install
cmake --build build --target install
```
- Libraries, headers and cmake config files will be installed to `install/`
- To build doxygen documentation, add `-DTGUY_BUILD_DOCS=ON` when configuring
- To build libtguy as a shared library, add `-DBUILD_SHARED_LIBS=ON`
- You can select unicode grapheme backend using `-DTGUY_UNICODE_LIBRARY=`:
- - `utf8proc` - around 350kb in size, stable and feature-complete unicode library
- - `wgrapheme` - minimal 22kb library, still under development, but should produce exactly the same results as `utf8proc`
- - `none` or `OFF` - a fallback which splits text on every codepoint will be used instead,  
    this causes complex symbols in strings such as `ab👨‍👩‍👧‍👦cd` to be incorrectly treated as multiple characters:  
    `['a', 'b', '👨', '\u200d', '👩', '\u200d', '👧', '\u200d', '👦', 'c', 'd']` rather than `['a', 'b', '👨‍👩‍👧‍👦', 'c', 'd']`.  
    This option is advised to be used in languages and runtimes already implementing own grapheme break libraries.  
    In this case, library user should split text into array of strings manually and use `tguy_from_arr()` or `tguy_from_cstr_arr()` families of constructors.
- For advanced manual configuration process and list of auxiliary options use `ccmake` or `CMake-GUI` instead of `cmake`
