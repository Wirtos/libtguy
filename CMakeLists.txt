cmake_minimum_required(VERSION 3.23)

include(cmake/PreventInSourceBuild.cmake)
prevent_in_source_build()

include(cmake/GetVersionFromMacro.cmake)
get_version_from_macro(
    OUTPUT TGUY_VERSION
    HEADER libtguy.h
    PREFIX TGUY_VER_
    VERSIONS MAJOR MINOR PATCH
)

project(tguy
    LANGUAGES C
    VERSION "${TGUY_VERSION}"
    DESCRIPTION "TrashGuy C library created to be easily usable from other languages with CFFI support.")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(BUILD_SHARED_LIBS "Build libtguy as dynamic library" OFF)
option(TGUY_USE_FASTCLEAR "Double arena memory usage to increase speed" OFF)
option(TGUY_BUILD_DOCS "Build doxygen docs" OFF)
option(TGUY_USE_UTF8PROC "Use utf8proc library for full unicode support. Legacy, use options available in TGUY_UNICODE_LIBRARY instead" OFF)
set(TGUY_UNICODE_LIBRARY "utf8proc" CACHE STRING
    "Select a unicode support backend")
set_property(CACHE TGUY_UNICODE_LIBRARY PROPERTY STRINGS
    "utf8proc" #[["libgrapheme" not yet supported]] "wgrapheme" "none")
if (TGUY_USE_UTF8PROC)
    message(WARNING "TGUY_USE_UTF8PROC is deprecated, use TGUY_UNICODE_LIBRARY='utf8proc'")
    set(TGUY_UNICODE_LIBRARY "utf8proc")
endif ()

message(STATUS "libtguy ${TGUY_VERSION}")

set(PACKAGE_NAME TGuy)
add_library(${PROJECT_NAME})
add_library(${PACKAGE_NAME}::TGuy ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PUBLIC
        FILE_SET HEADERS
        FILES libtguy.h
    PRIVATE
        libtguy.c
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    LIBTGUY_EXPORTS
)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        LIBTGUY_SHARED_DEFINE
    )
endif ()


# link libm for sqrt where applicable
include(CheckLibraryExists)
check_library_exists(m sqrt "" LIBM_EXISTS)
if (LIBM_EXISTS)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif ()

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        EXPORT_NAME ${PACKAGE_NAME}
        PREFIX "lib"
        SOVERSION ${TGUY_VERSION_MAJOR}.${TGUY_VERSION_MINOR}
)

if (TGUY_USE_FASTCLEAR)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TGUY_FASTCLEAR)
endif ()

if ("${TGUY_UNICODE_LIBRARY}" STREQUAL "utf8proc")
    message(STATUS "using utf8proc for unicode support")
    find_package(utf8proc CONFIG QUIET)
    if (NOT utf8proc_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            utf8proc
            GIT_REPOSITORY https://github.com/JuliaStrings/utf8proc
            GIT_TAG 060824da775b09aed8402a0019db5430b3c39732
        )
        set(BUILD_SHARED_LIBS OFF)
        FetchContent_MakeAvailable(utf8proc)
        unset(BUILD_SHARED_LIBS)
    endif ()

    target_compile_definitions(${PROJECT_NAME} PRIVATE TGUY_USE_UTF8PROC)
    target_link_libraries(${PROJECT_NAME} PRIVATE utf8proc::utf8proc)
elseif ("${TGUY_UNICODE_LIBRARY}" STREQUAL "libgrapheme")
    message(STATUS "using libgrapheme for unicode support")
    find_package(grapheme QUIET CONFIG)
    if (NOT grapheme_FOUND)
        include(FetchContent)
        # Fallback: fetch and build
        FetchContent_Declare(
            grapheme
            GIT_REPOSITORY https://git.suckless.org/libgrapheme
            GIT_TAG master
        )
        set(BUILD_SHARED_LIBS OFF)
        FetchContent_MakeAvailable(grapheme)
        unset(BUILD_SHARED_LIBS)
    endif ()

    target_compile_definitions(${PROJECT_NAME} PRIVATE TGUY_USE_LIBGRAPHEME)
    target_link_libraries(${PROJECT_NAME} PRIVATE grapheme)
elseif ("${TGUY_UNICODE_LIBRARY}" STREQUAL "wgrapheme")
    message(STATUS "using wgrapheme for unicode support")
    find_package(wgrapheme CONFIG QUIET)
    if (NOT wgrapheme_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            wgrapheme
            GIT_REPOSITORY https://github.com/Wirtos/wgrapheme
            GIT_TAG dcb023029dfa6c4ede71e9b9459bc4515f7b7486
        )
        set(BUILD_SHARED_LIBS OFF)
        FetchContent_MakeAvailable(wgrapheme)
        unset(BUILD_SHARED_LIBS)
    endif ()

    target_compile_definitions(${PROJECT_NAME} PRIVATE TGUY_USE_WGRAPHEME)
    target_link_libraries(${PROJECT_NAME} PRIVATE wgrapheme::wgrapheme)
endif ()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Save library targets into ${PACKAGE_NAME}Targets export set as a TGuy component
# This doesn't install any real files
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PACKAGE_NAME}Targets
    COMPONENT TGuy
    FILE_SET HEADERS
)

# install the file set as an actual ${PACKAGE_NAME}Targets.cmake and ${PACKAGE_NAME}Targets-${CMAKE_BUILD_TYPE}.cmake
install(
    EXPORT ${PACKAGE_NAME}Targets
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PACKAGE_NAME}"
    NAMESPACE ${PACKAGE_NAME}::
)

# Generate ${PACKAGE_NAME}Config.cmake config file
# From a template file, all this file does is includes ${PACKAGE_NAME}Targets.cmake and checks if TGuy component is present inside
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PACKAGE_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PACKAGE_NAME}"
)

# A file loaded by find_package, tells
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}Version.cmake"
    COMPATIBILITY SameMajorVersion
)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}Version.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PACKAGE_NAME}"
)

# generate and install .cps from ${PACKAGE_NAME}Targets export set
if (CMAKE_VERSION VERSION_GREATER_EQUAL "4.3")
    install(
        PACKAGE_INFO ${PACKAGE_NAME}
        EXPORT ${PACKAGE_NAME}Targets
        VERSION ${PROJECT_VERSION}
        COMPAT_VERSION 0.7.0 # tguy_sprint was introduced
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PACKAGE_NAME}"
    )
endif ()

if (EMSCRIPTEN)
    add_subdirectory(bindings/js)
endif ()

if (TGUY_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    set(TGUY_DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doxygen" CACHE PATH
        "Default doxygen directory to output docs to")
    set(DOXYGEN_OUTPUT_DIRECTORY "${TGUY_DOXYGEN_OUTPUT_DIRECTORY}")
    set(DOXYGEN_EXTRACT_STATIC YES)
    doxygen_add_docs(${PROJECT_NAME}_docs
        libtguy.c
        libtguy.h
        ALL
    )

    install(
        DIRECTORY "${TGUY_DOXYGEN_OUTPUT_DIRECTORY}"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
        COMPONENT docs)
endif ()