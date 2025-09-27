# Cross-platform build (server only)

This repository includes a CMake configuration to build the server (cpp-httplib + standard C++), which is portable across Windows, Linux and macOS. The client uses EasyX (Windows-only) and is not included in the CMake build.

## Prerequisites
- CMake >= 3.16
- A C++17 compiler
  - Windows: MSVC (Visual Studio) or MinGW-w64
  - Linux: GCC or Clang
  - macOS: Apple Clang (Xcode Command Line Tools)

## Configure & Build

Windows (MSVC x64):

```bat
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..\Pawsome-Adventure
cmake --build . --config Release --target pawsome_server
```

Linux/macOS:

```sh
mkdir -p build && cd build
cmake ../Pawsome-Adventure -DCMAKE_BUILD_TYPE=Release
cmake --build . --target pawsome_server -j
```

The server binary will be located under `build/` subdirectories depending on generator.

## Notes
- cpp-httplib is fetched automatically via `FetchContent` by default. You can disable it with `-DUSE_FETCHCONTENT_HTTPLIB=OFF` and provide your own include path (e.g., via system packages).
- On Windows we link `ws2_32` implicitly; on POSIX platforms `Threads::Threads` pulls in pthread as needed.
- The server reads `server.ini` from the working directory. Ensure this file exists (generated at first run) and contains valid values (e.g., `text_file=resources/text.txt`).
