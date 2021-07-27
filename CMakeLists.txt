cmake_minimum_required(VERSION 3.12)

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

set(TGUY_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

find_package(UTF8Proc QUIET)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/install" CACHE PATH "" FORCE)
endif ()

add_library(${PROJECT_NAME} src/libtguy.c)
add_library(TGuy::TGuy ALIAS ${PROJECT_NAME})
if ("${CMAKE_C_COMPILER_ID}" MATCHES "MSVC" OR "${CMAKE_C_SIMULATE_ID}" MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME}
            PRIVATE
            $<$<CONFIG:Release>:-fp:fast> -W4 -WX
            )
elseif ("${CMAKE_C_COMPILER_ID}" MATCHES "GNU|Intel|Clang" OR "${CMAKE_C_SIMULATE_ID}" MATCHES "GNU")
    target_compile_options(${PROJECT_NAME}
            PRIVATE
            $<$<CONFIG:Release>:-fno-math-errno> $<$<C_COMPILER_ID:GNU,Clang>:-pedantic-errors> -Wall -Wextra -Werror
            )
endif ()

if (TGUY_USE_FASTCLEAR)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TGUY_FASTCLEAR)
endif ()

if (NOT UTF8PROC_FOUND)
    include(FetchContent)
    message(WARNING "UTF8PROC not found, using git version...")

    FetchContent_Declare(
            utf8proc
            GIT_REPOSITORY https://github.com/Wirtos/utf8proc
    )

    FetchContent_GetProperties(utf8proc)
    if (NOT utf8proc_POPULATED)
        FetchContent_Populate(utf8proc)
    endif ()

    add_subdirectory(${utf8proc_SOURCE_DIR})

    get_target_property(UTF8PROC_INCLUDES utf8proc INTERFACE_INCLUDE_DIRECTORIES)
    # link utf8proc object directly
    target_sources(${PROJECT_NAME} PRIVATE $<TARGET_OBJECTS:utf8proc>)
    target_include_directories(${PROJECT_NAME} PRIVATE ${UTF8PROC_INCLUDES})
else ()
    target_link_libraries(${PROJECT_NAME} PRIVATE utf8proc)
endif ()

include(CheckLibraryExists)
check_library_exists(m sqrt "" LIBM_EXISTS)
if (LIBM_EXISTS)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif ()

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})

include(GNUInstallDirs)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        )

# export tguy target as TGuy
# declare headers to be installed
set_property(TARGET ${PROJECT_NAME}
        PROPERTY PUBLIC_HEADER
        include/libtguy.h
        "${CMAKE_CURRENT_BINARY_DIR}/libtguy_export.h"
        )
set_target_properties(${PROJECT_NAME} PROPERTIES
        EXPORT_NAME TGuy
        PREFIX ""
        )

install(TARGETS ${PROJECT_NAME}
        EXPORT TGuyConfig
        COMPONENT TGuy
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
        )

export(TARGETS
        ${PROJECT_NAME}
        NAMESPACE TGuy::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/TGuyConfig.cmake"
        )

install(EXPORT TGuyConfig
        DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake"
        NAMESPACE TGuy::
        )

if (${TGUY_BUILD_DOCS})
    find_package(Doxygen REQUIRED)

    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doxygen")
    set(DOXYGEN_EXTRACT_STATIC YES)
    doxygen_add_docs(
            _libtguy_docs
            include
            src
            COMMENT "Generate docs"
    )
    add_custom_target(libtguy_docs ALL DEPENDS _libtguy_docs COMMAND "" BYPRODUCTS "${DOXYGEN_OUTPUT_DIRECTORY}")
    install(DIRECTORY "${DOXYGEN_OUTPUT_DIRECTORY}" TYPE DOC)

endif ()
