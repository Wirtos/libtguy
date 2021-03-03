cmake_minimum_required(VERSION 3.10)

include(cmake/GetVersionString.cmake)
get_version_string(include/libtguy.h "TGUY" TGUY_VER)

project(libtguy LANGUAGES C VERSION ${TGUY_VER}
        HOMEPAGE_URL "https://github.com/Wirtos/libtguy"
        DESCRIPTION "TrashGuy C library created to be easily usable from other languages with CFFI support.")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(BUILD_SHARED_LIBS "Whether to build a dynamic library" OFF)
option(TGUY_USE_FASTCLEAR "Whether to double space usage to increase speed" ON)
option(TGUY_BUILD_DOCS "Whether to build doxygen docs" OFF)

message(DEBUG "libtguy ${TGUY_VER}")

include(cmake/PreventInSourceBuild.cmake)
prevent_in_source_build()

if (${CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT})
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/install" CACHE PATH "" FORCE)
endif ()

add_library(${PROJECT_NAME} src/libtguy.c)
if ("${CMAKE_C_COMPILER_ID}" MATCHES "MSVC" OR "${CMAKE_C_SIMULATE_ID}" STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME}
            PRIVATE
            $<$<CONFIG:Release>:-fp:fast> -W4 -WX)
elseif (${CMAKE_C_COMPILER_ID} MATCHES "GNU|Intel|Clang" OR ${CMAKE_C_SIMULATE_ID} MATCHES "GNU")
    target_compile_options(${PROJECT_NAME}
            PRIVATE
            $<$<CONFIG:Release>:-fno-math-errno> $<$<C_COMPILER_ID:GNU,Clang>:-pedantic-errors> -Wall -Wextra -Werror)
endif ()

add_library(TGuy::TGuy ALIAS ${PROJECT_NAME})

if (TGUY_USE_FASTCLEAR)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TGUY_FASTCLEAR)
endif ()

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})

include(CheckLibraryExists)
check_library_exists(m sqrt "" LIBM_EXISTS)
if (LIBM_EXISTS)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif ()

include(GNUInstallDirs)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

# export tguy target as TGuy
# declare headers to be installed
set_property(TARGET ${PROJECT_NAME}
        PROPERTY PUBLIC_HEADER
        include/libtguy.h
        "${CMAKE_CURRENT_BINARY_DIR}/libtguy_export.h")
set_target_properties(${PROJECT_NAME} PROPERTIES
        EXPORT_NAME TGuy
        PREFIX "")

install(TARGETS
        ${PROJECT_NAME}
        EXPORT TGuyConfig)

export(TARGETS
        ${PROJECT_NAME}
        NAMESPACE TGuy::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/TGuyConfig.cmake"
        )

install(EXPORT
        TGuyConfig
        DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake"
        NAMESPACE TGuy::
        )

if (${TGUY_BUILD_DOCS})
    find_package(Doxygen REQUIRED)
    if (${DOXYGEN_FOUND})
        set(DOXYGEN_GENERATE_HTML TRUE CACHE BOOL "Whether to generate HTML when building docs" FORCE)
        set(DOXYGEN_GENERATE_LATEX FALSE CACHE BOOL "Whether to generate LaTeX when building docs" FORCE)
        set(DOXYGEN_GENERATE_MAN FALSE CACHE BOOL "Whether to generate manpage when building docs" FORCE)
        set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doxygen")
        doxygen_add_docs(
                libtguy_docs
                include
                COMMENT "Generate docs"
        )
        add_custom_command(OUTPUT "${DOXYGEN_OUTPUT_DIRECTORY}" DEPENDS libtguy_docs)
        install(DIRECTORY "${DOXYGEN_OUTPUT_DIRECTORY}" TYPE DOC)
    endif ()
endif ()
