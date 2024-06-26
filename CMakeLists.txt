cmake_minimum_required(VERSION 3.8)
project(xq)

set(XQ_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(XQ_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(XQ_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)


# Compilation options
option(XQ_BUILD_SHARED_LIBS "Build XQ shared library." ON)
option(XQ_BUILD_STATIC_LIBS "Build XQ static library (default if BUILD_SHARED_LIBS is OFF)." ON)
option(XQ_STATIC_DEPENDENCIES "link statically with XQ dependencies" OFF)

# Versioning
# ==========

# Project version
file(STRINGS "${XQ_INCLUDE_DIR}/xq/xq.hpp" xq_version_defines
     REGEX "#define XQ_VERSION_(MAJOR|MINOR|PATCH)")
foreach(ver ${xq_version_defines})
    if(ver MATCHES "#define XQ_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
        set(XQ_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif()
endforeach()
set(XQ_VERSION
    ${XQ_VERSION_MAJOR}.${XQ_VERSION_MINOR}.${XQ_VERSION_PATCH})
message(STATUS "xq version: v${XQ_VERSION}")

# Binary version
# See the following URL for explanations about the binary versionning
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html#Updating-version-info
file(STRINGS "${XQ_INCLUDE_DIR}/xq/xq.hpp" xq_version_defines
    REGEX "#define XQ_BINARY_(CURRENT|REVISION|AGE)")
foreach(ver ${xq_version_defines})
    if(ver MATCHES "#define XQ_BINARY_(CURRENT|REVISION|AGE) +([^ ]+)$")
        set(XQ_BINARY_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif()
endforeach()
set(XQ_BINARY_VERSION
    ${XQ_BINARY_CURRENT}.${XQ_BINARY_REVISION}.${XQ_BINARY_AGE})
message(STATUS "xq binary version: v${XQ_BINARY_VERSION}")

# Dependencies
# ============

set(nlohmann_json_REQUIRED_VERSION 3.11.3)
set(xeus_zmq_REQUIRED_VERSION 3.0)

find_package(Qt5 COMPONENTS Core REQUIRED)

if (NOT TARGET xeus-zmq AND NOT TARGET xeus-zmq-static)
    find_package(xeus-zmq ${xeus_zmq_REQUIRED_VERSION} REQUIRED)
endif ()

if (NOT TARGET nlohmann_json)
    find_package(nlohmann_json ${nlohmann_json_REQUIRED_VERSION} REQUIRED)
endif ()

# Source files
# ============

set(XQ_HEADERS
    ${XQ_INCLUDE_DIR}/xq/xq.hpp
    ${XQ_INCLUDE_DIR}/xq/xq_server.hpp
    ${XQ_INCLUDE_DIR}/xq/xq_qt_poller.hpp
    )

set(XQ_SOURCES
    ${XQ_SOURCE_DIR}/xq_server.cpp
    ${XQ_SOURCE_DIR}/xq_qt_poller.cpp
   )

# Targets and link
# ================

include(CheckCXXCompilerFlag)

string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

macro(xq_create_target target_name linkage output_name)
    string(TOUPPER "${linkage}" linkage_upper)

    if (NOT ${linkage_upper} MATCHES "^(SHARED|STATIC)$")
        message(FATAL_ERROR "Invalid library linkage: ${linkage}")
    endif ()

    # Output
    # ======

    add_library(${target_name} ${linkage_upper} ${XQ_SOURCES} ${XQ_HEADERS})

    target_include_directories(
        ${target_name}
        PUBLIC $<BUILD_INTERFACE:${XQ_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
    )

    target_link_libraries(
        ${target_name}
        PUBLIC nlohmann_json::nlohmann_json
        PUBLIC xeus-zmq
        PUBLIC Qt5::Core
    )

    if (UNIX)
        # CMake does not compute the version number of so files as libtool
        # does on Linux. Strictly speaking, we should exclude FreeBSD and
        # Apple from this, but that would require having different version
        # numbers depending on the platform. We prefer to follow the
        # libtool pattern everywhere.
        math(EXPR XQ_BINARY_COMPATIBLE "${XQ_BINARY_CURRENT} - ${XQ_BINARY_AGE}")
        set_target_properties(
            ${target_name}
            PROPERTIES
            PUBLIC_HEADER "${XQ_HEADERS}"
            COMPILE_DEFINITIONS "XQ_EXPORTS"
            PREFIX ""
            VERSION "${XQ_BINARY_COMPATIBLE}.${XQ_BINARY_REVISION}.${XQ_BINARY_AGE}"
            SOVERSION ${XQ_BINARY_COMPATIBLE}
            OUTPUT_NAME "lib${output_name}"
        )
    else()
        set_target_properties(
            ${target_name}
            PROPERTIES
            PUBLIC_HEADER "${XQ_HEADERS}"
            COMPILE_DEFINITIONS "XQ_EXPORTS"
            PREFIX ""
            VERSION ${XQ_BINARY_VERSION}
            SOVERSION ${XQ_BINARY_CURRENT}
            OUTPUT_NAME "lib${output_name}"
        )
    endif()

    # Compilation flags
    # =================

    target_compile_features(${target_name} PRIVATE cxx_std_17)

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
        CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
        CMAKE_CXX_COMPILER_ID MATCHES "Intel")

        target_compile_options(${target_name} PUBLIC -Wunused-parameter -Wextra -Wreorder)

        message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
    endif()

    if (${linkage_upper} STREQUAL "STATIC")
        target_compile_definitions(${target_name} PUBLIC XQ_STATIC_LIB)
    endif ()

    if (MSVC)
        target_compile_definitions(${target_name} PUBLIC -DNOMINMAX)
        target_compile_options(${target_name} PUBLIC /DGUID_WINDOWS /MP /bigobj)
        target_compile_options(${target_name} PUBLIC /wd4251 /wd4996)
    endif ()

    if (XQ_STATIC_DEPENDENCIES AND CMAKE_DL_LIBS)
        target_link_libraries(${target_name} PRIVATE ${CMAKE_DL_LIBS} util rt)
    endif ()

endmacro()

set(xq_targets "")

if (XQ_BUILD_SHARED_LIBS)
    xq_create_target(xq SHARED xq)
    if(CMAKE_TARGET_SYSTEM MATCHES "Linux" AND OPENSSL_USE_STATIC_LIBS)
        # Do not reexport OpenSSL symbols from xq, for libraries
        #   Prevents conflicts with other versions of OpenSSL
        #   loaded in the same process namespace, which can cause
        #   crashes if the versions are not compatible.
        set_target_properties(xq PROPERTIES LINK_FLAGS "-Wl,--exclude-libs,libcrypto.a")
    endif()
    list(APPEND xq_targets xq)
endif ()

if (XQ_BUILD_STATIC_LIBS)
    # On Windows, a static library should use a different output name
    # to avoid the conflict with the import library of a shared one.
    if (CMAKE_HOST_WIN32)
        xq_create_target(xq-static STATIC xq-static)
    else ()
        xq_create_target(xq-static STATIC xq)
    endif ()

    list(APPEND xq_targets xq-static)
endif ()


# Installation
# ============

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(XQ_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE STRING "install path for xqConfig.cmake")

install(TARGETS ${xq_targets}
        EXPORT ${PROJECT_NAME}-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xq)

# Makes the project importable from the build directory
export(EXPORT ${PROJECT_NAME}-targets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")

# Configure 'xqConfig.cmake' for a build tree
set(XQ_CONFIG_CODE "####### Expanded from \@XQ_CONFIG_CODE\@ #######\n")
set(XQ_CONFIG_CODE "${XQ_CONFIG_CODE}set(CMAKE_MODULE_PATH \"${CMAKE_CURRENT_SOURCE_DIR}/cmake;\${CMAKE_MODULE_PATH}\")\n")
set(XQ_CONFIG_CODE "${XQ_CONFIG_CODE}##################################################")
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${PROJECT_BINARY_DIR})

# Configure 'xqConfig.cmake' for an install tree
set(XQ_CONFIG_CODE "")
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${XQ_CMAKECONFIG_INSTALL_DIR})


write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
                                 VERSION ${XQ_VERSION}
                                 COMPATIBILITY AnyNewerVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
              DESTINATION ${XQ_CMAKECONFIG_INSTALL_DIR})
install(EXPORT ${PROJECT_NAME}-targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${XQ_CMAKECONFIG_INSTALL_DIR})
