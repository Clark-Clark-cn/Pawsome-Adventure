cmake_minimum_required(VERSION 3.16)

# Subproject builds server by default; client is optional and Windows-only.
project(PawsomeAdventureSub LANGUAGES CXX)

# Options inherited from top-level
option(BUILD_SERVER "Build the cross-platform server" ON)
option(BUILD_CLIENT "Build the Windows-only client" OFF)
option(USE_FETCHCONTENT_HTTPLIB "Fetch cpp-httplib via FetchContent (if not provided by system)" ON)

find_package(Threads REQUIRED)

# Common warnings helper
function(enable_warnings tgt)
    if (MSVC)
        target_compile_options(${tgt} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${tgt} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

if (BUILD_SERVER)
    add_executable(pawsome_server server.cpp)
    target_include_directories(pawsome_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    if (USE_FETCHCONTENT_HTTPLIB)
        include(FetchContent)
        FetchContent_Declare(
            cpp_httplib
            GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
            GIT_TAG v0.15.3
        )
        FetchContent_MakeAvailable(cpp_httplib)
        target_include_directories(pawsome_server PRIVATE ${cpp_httplib_SOURCE_DIR})
    else()
        find_path(HTTPLIB_INCLUDE_DIR NAMES httplib.h
            HINTS ${VCPKG_INSTALLED_DIR}/x64-windows/include
                  ${VCPKG_INSTALLED_DIR}/x86-windows/include
                  ${VCPKG_INSTALLED_DIR}/x64-linux/include
                  ${VCPKG_INSTALLED_DIR}/arm64-osx/include
                  ${VCPKG_INSTALLED_DIR}/x64-osx/include
                  ${CMAKE_INSTALL_PREFIX}/include
                  /usr/include /usr/local/include /opt/homebrew/include /opt/local/include)
        if (NOT HTTPLIB_INCLUDE_DIR)
            message(FATAL_ERROR "Could not find httplib.h. Set -DUSE_FETCHCONTENT_HTTPLIB=ON or install via vcpkg/apt/brew and set HTTPLIB_INCLUDE_DIR.")
        endif()
        target_include_directories(pawsome_server PRIVATE ${HTTPLIB_INCLUDE_DIR})
    endif()

    target_link_libraries(pawsome_server PRIVATE Threads::Threads)
    if (WIN32)
        target_link_libraries(pawsome_server PRIVATE ws2_32)
    endif()
    enable_warnings(pawsome_server)

    # Runtime resources copy
    set(SERVER_RUNTIME_FILES
        ${CMAKE_CURRENT_LIST_DIR}/../server/server.ini
        ${CMAKE_CURRENT_LIST_DIR}/../server/text.txt)
    foreach(f IN LISTS SERVER_RUNTIME_FILES)
        if (EXISTS ${f})
            add_custom_command(TARGET pawsome_server POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${f} $<TARGET_FILE_DIR:pawsome_server>
                VERBATIM)
        endif()
    endforeach()

    include(GNUInstallDirs)
    install(TARGETS pawsome_server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    foreach(f IN LISTS SERVER_RUNTIME_FILES)
        if (EXISTS ${f})
            install(FILES ${f} DESTINATION ${CMAKE_INSTALL_BINDIR})
        endif()
    endforeach()
endif()

# Placeholder for client target (Windows-only). If未来需要，我们可根据 EasyX/依赖添加实现。
if (BUILD_CLIENT AND WIN32)
    message(STATUS "BUILD_CLIENT is ON, but client target is not yet defined due to platform-specific GUI deps.")
endif()

