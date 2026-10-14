# Lua 5.4.8 (vendored)

Source: https://www.lua.org/ftp/lua-5.4.8.tar.gz
SHA-256: 4f18ddae154e793e46eeab727c59ef1c0c0c2b744e7b94219710d76f530629ae
Licence: MIT (see the notice at the end of `lua.h`).

Unmodified files from the tarball's `src/` directory. The standalone
interpreter and compiler (`lua.c`, `luac.c`) and the libraries the reader
never opens (`io`, `os`, `package`, `debug`, plus `linit.c`) are left out.

The reader fork builds these sources as C++ (see `externals/CMakeLists.txt`)
so Lua errors unwind as C++ exceptions through the bindings in
`src/core/reader/lua_api.cpp`.
