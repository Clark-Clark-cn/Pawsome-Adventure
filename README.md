# Pawsome Adventure (Hajimi Adventure)

A multiplayer (including team mode) text racing game, featuring a cross-platform HTTP server (C++17 + cpp-httplib) and a Windows client (based on EasyX, not yet included in CMake). The server handles text distribution, heartbeat synchronization, and progress aggregation; the client handles rendering and interaction.

## **start game**
- Run the server
- Run the client (at least 2 clients or 2 groups of clients)

## Features

- Cross-platform server: Windows / Linux / macOS (CMake build)
- Team/Solo modes, configurable team size
- Text segmented and distributed by team members (different segments of the same text)
- Heartbeat synchronization, automatic cleanup of timed-out players
- Simple REST API, easy to integrate or self-test
- GitHub Actions CI (three-platform build + artifact upload)

## Repository Structure

```
Pawsome-Adventure/
├─ CMakeLists.txt                # Top-level CMake (forwards to subdirectory)
├─ README-cmake.md               # English build instructions (server-only)
├─ README.md / README_cn.md      # Project description (you're reading this)
├─ vcpkg.json                    # Dependency declaration (cpp-httplib)
├─ .github/workflows/ci.yml      # CI: Windows/Linux/macOS build and upload artifacts
├─ Pawsome-Adventure/            # Source code (server/client, shared headers)
│  ├─ CMakeLists.txt             # Subproject CMake (builds server by default)
│  ├─ server.cpp                 # Server entry (cross-platform)
│  ├─ client.cpp                 # Client (Windows + EasyX)
│  ├─ config.h                   # Simple config system (shared by server/client)
│  ├─ ... other headers (animation/rendering/path/timing, etc.)
├─ server/                       # Server default resources
│  ├─ server.ini                 # Server config (saved/updated on first run)
│  └─ text.txt                   # Demo text
└─ resources/                    # Client resources (images/fonts/audio) (Windows)
```

## Prerequisites

- CMake 3.16+
- C++17 compiler
  - Windows: Visual Studio 2022 (MSVC) or MinGW-w64
  - Linux: GCC / Clang
  - macOS: Apple Clang (Xcode Command Line Tools)
- Dependencies: cpp-httplib (header-only)
  - Default: Auto-fetched via FetchContent
  - Or install via vcpkg/apt/brew and provide include path

## Quick Start (Building Server)

Builds only the server target `pawsome_server` by default.

- Windows (MSVC, x64)

```cmd
cmake -S . -B build -A x64 -DUSE_FETCHCONTENT_HTTPLIB=ON
cmake --build build --config Release -- /m
```

- Linux/macOS

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUSE_FETCHCONTENT_HTTPLIB=ON
cmake --build build -j
```

After build, the `pawsome_server` executable directory will auto-copy `server/server.ini` and `server/text.txt` for easy running.

Optional: Use vcpkg (instead of FetchContent)

- Windows (example)

```cmd
cmake -S . -B build -A x64 -DUSE_FETCHCONTENT_HTTPLIB=OFF -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release -- /m
```

- Linux/macOS (example)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUSE_FETCHCONTENT_HTTPLIB=OFF -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build -j
```

## Running Server

Run in the build output directory (containing `server.ini`/`text.txt`):

- Windows

```cmd
.\pawsome_server.exe
```

- Linux/macOS

```sh
./pawsome_server
```

Listens on `0.0.0.0:8888` by default. First run will write default config back to `server.ini`.

## Configuration (server.ini)

Example:

```
port=8888
team_mode=false
text_file=text.txt
heartbeat_timeout=10
team_size=2
server=http://127.0.0.1:8888
```

- port: Listening port
- team_mode: Enable team mode (true/false)
- team_size: Team size (>=1)
- heartbeat_timeout: Heartbeat timeout in seconds
- text_file: Text file to distribute (relative to working directory)
- server: Client default server address (for client use)

Note: `text_file` path is relative to the runtime working directory. If starting from another directory, use absolute path or ensure file exists.

## REST API

Server provides simple API via cpp-httplib (Content-Type: text/plain):

- POST `/login`
  - Response: `<id>,<teamID>,<memberIndex>,<teamSize>`
  - Description: Returns player ID and assigned team info; server assigns based on `team_mode`/`team_size`

- POST `/queryText` (optional param: `id`)
  - No `id`: Returns full text
  - With `id`: If in team mode, returns the segment for that member; else full text

- POST `/sync` (params: `id`, `progress`)
  - Reports player progress (integer, custom semantics)
  - Response: List like `pid:progress:teamID:segmentLen;...`
  - Also refreshes heartbeat; server cleans up players exceeding `heartbeat_timeout`

- POST `/exit` (param: `id`)
  - Exit and remove player, returns `ok`

- GET `/mode`
  - Response: `team,<size>` or `solo,<size>`

- POST `/setMode` (param: `mode` as `team|solo|true|false|1|0`; optional `teamSize`)
  - Changes mode and team size; clears online players and resets IDs
  - Syncs back to `server.ini`

Quick self-test (examples)

```sh
# Login (returns id,teamID,memberIndex,teamSize)
curl -X POST http://127.0.0.1:8888/login

# Query text (with id returns segment)
curl -X POST "http://127.0.0.1:8888/queryText?id=1"

# Report progress
curl -X POST "http://127.0.0.1:8888/sync?id=1&progress=123"

# Query/set mode
curl http://127.0.0.1:8888/mode
curl -X POST "http://127.0.0.1:8888/setMode?mode=team&teamSize=3"
```

Security note: This service is for local/intranet demo only, no auth/rate limiting/persistence; do not expose to public internet.

## Client (Windows/EasyX)

- Source: `Pawsome-Adventure/client.cpp` and related headers, `resources/` assets
- Dependencies: EasyX, WinMM/MSIMG32 (see `#pragma comment` in `util.h`)
- Not yet included in CMake build; for building, configure in VS manually or let me know to add `BUILD_CLIENT` switch (Windows only)

## Continuous Integration (CI)

- Workflow: `.github/workflows/ci.yml`
- Triggers: push / pull_request to `main`, or manual
- Platforms: Windows/Linux/macOS
- Artifact upload:
  - Windows: `pawsome_server.exe` + `server.ini` + `text.txt`
  - Linux/macOS: `pawsome_server` + `server.ini` + `text.txt`
- Build options: Default `-DUSE_FETCHCONTENT_HTTPLIB=ON`

Download corresponding platform artifacts from the repository Actions page.

## FAQ

- Build fails, can't find `httplib.h`
  - Default `USE_FETCHCONTENT_HTTPLIB=ON` auto-fetches; if off, install via vcpkg/apt/brew and provide include path (or use vcpkg toolchain)
- Runtime error "Failed to open text: ..."
  - Check `server.ini` `text_file` path, ensure relative to working directory, or use absolute path
- Port occupied
  - Change `port` in `server.ini`, or kill the process using that port
- Linux/macOS missing thread library
  - CMake handles via `find_package(Threads)`; if fails, ensure toolchain/headers are complete
- Windows console garbled
  - Try switching terminal encoding or view English logs only (current logs are mostly English/numbers)

## Development and Contribution

- Use CMake for unified builds; recommend `-DUSE_FETCHCONTENT_HTTPLIB=ON`
- PRs should include description and platform verification
- If integrating client into CMake, please specify EasyX acquisition (install/include paths/libs)

## License

No license declared yet (all rights reserved). Contact repository owner for redistribution or commercial use.