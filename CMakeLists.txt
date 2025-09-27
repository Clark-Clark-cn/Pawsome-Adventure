cmake_minimum_required(VERSION 3.16)

# Top-level project
project(PawsomeAdventure LANGUAGES CXX)

# Global options
option(BUILD_SERVER "Build the cross-platform server" ON)
option(BUILD_CLIENT "Build the Windows-only client (requires EasyX or similar)" OFF)
set(USE_FETCHCONTENT_HTTPLIB ON CACHE BOOL "Fetch cpp-httplib via FetchContent if not provided by system")

# Helpful defaults for IDEs/tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Delegate to subdirectory which contains targets
add_subdirectory(Pawsome-Adventure)


