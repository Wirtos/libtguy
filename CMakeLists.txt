cmake_minimum_required(VERSION 3.15)

include(cmake/PreventInSourceBuild.cmake)
prevent_in_source_build()

include(cmake/GetVersionFromMacro.cmake)
get_version_from_macro(
        HEADER include/libtguy.h
        PREFIX TGUY_VER_
        VERSIONS MAJOR MINOR PATCH
        OUTPUT TGUY_VERSION)

set(PACKAGE_NAME TGuy)
project(libtguy
        LANGUAGES C
        VERSION "${TGUY_VERSION}"
        DESCRIPTION "TrashGuy C library created to be easily usable from other languages with CFFI support.")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(BUILD_SHARED_LIBS "Whether to build a dynamic library" OFF)
option(TGUY_USE_FASTCLEAR "Whether to double space usage to increase speed" ON)
option(TGUY_BUILD_DOCS "Whether to build doxygen docs" OFF)

message(STATUS "libtguy ${TGUY_VERSION}")


add_library(${PROJECT_NAME} src/libtguy.c)
add_library(${PACKAGE_NAME}::TGuy ALIAS ${PROJECT_NAME})

if ("${CMAKE_C_COMPILER_ID}" MATCHES "MSVC" OR "${CMAKE_C_SIMULATE_ID}" MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME}
            PRIVATE
            $<$<CONFIG:Release>:-fp:fast> -W4
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

find_package(UTF8Proc CONFIG QUIET)

if (UTF8Proc_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE utf8proc)
else ()
    include(FetchContent)
    message(STATUS "UTF8Proc not found, embedding git version...")

    FetchContent_Declare(
            utf8proc
            GIT_REPOSITORY https://github.com/Wirtos/utf8proc
            GIT_TAG 7891a34766173c8be2b309165161bbdd301abfff
    )

    FetchContent_MakeAvailable(utf8proc)

    # requires cmake 3.15
    target_sources(${PROJECT_NAME} PRIVATE $<TARGET_OBJECTS:utf8proc>)
    target_include_directories(${PROJECT_NAME} PRIVATE $<TARGET_PROPERTY:utf8proc,INTERFACE_INCLUDE_DIRECTORIES>)
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
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>"

        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
        )

# export tguy target as TGuy
# declare headers to be installed
set_property(TARGET ${PROJECT_NAME}
        PROPERTY PUBLIC_HEADER
        include/libtguy.h
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_export.h"
        )
set_target_properties(${PROJECT_NAME} PROPERTIES
        EXPORT_NAME TGuy
        PREFIX ""
        )

if (EMSCRIPTEN)
    # this is ugly but emscripten toolchain is dumb and doesn't have an option to build library target as wasm
    # gather all the properties required to build the lib and make it an executable
    add_executable(${PROJECT_NAME}_wasm $<TARGET_PROPERTY:${PROJECT_NAME},SOURCES>)
    target_link_libraries(${PROJECT_NAME}_wasm PRIVATE $<TARGET_PROPERTY:${PROJECT_NAME},LINK_LIBRARIES>)
    target_include_directories(${PROJECT_NAME}_wasm PRIVATE $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>)
    target_link_options(${PROJECT_NAME}_wasm PRIVATE
            --closure=$<IF:$<CONFIG:Debug>,0,1>
            -sEXPORTED_RUNTIME_METHODS=[cwrap,lengthBytesUTF8,stringToUTF8]
            -sEXPORTED_FUNCTIONS=[_malloc,_free]
            -sMODULARIZE=1
            --extern-post-js "${CMAKE_CURRENT_SOURCE_DIR}/bindings/libtguy.js"
            -sEXPORT_NAME=TGuyModule
            -sCLOSURE_WARNINGS=warn)
    # cmake emits EXPORT as __attribute__((default)), which doesn't actually keep the function alive in emscripten
    target_compile_definitions(${PROJECT_NAME}_wasm PRIVATE -DLIBTGUY_EXPORT=__attribute__\(\(used\)\))
    set(TGUY_WASM_BASE_NAME $<TARGET_FILE_PREFIX:${PROJECT_NAME}>$<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>)
    set_target_properties(${PROJECT_NAME}_wasm PROPERTIES OUTPUT_NAME ${TGUY_WASM_BASE_NAME})
    # cmake (or emscripten toolchain) is dumb and doesn't know executable can emit few files so install them manually
    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${TGUY_WASM_BASE_NAME}.wasm"
            "${CMAKE_CURRENT_BINARY_DIR}/${TGUY_WASM_BASE_NAME}.js"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}")
endif ()

# install targets' resulting files
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PACKAGE_NAME}Config
        COMPONENT TGuy
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}"
        )

# write export config into binary dir in order to find is as an external project without installing
export(EXPORT ${PACKAGE_NAME}Config
        NAMESPACE ${PACKAGE_NAME}::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}Config.cmake"
        )

# install export configs into install dir to find it in a project setting ${PROJECT_NAME}_DIR variable to datadir/cmake
install(EXPORT ${PACKAGE_NAME}Config
        DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake"
        NAMESPACE ${PACKAGE_NAME}::
        )

# write version config file into binary dir in order to find is as a subdirectory project
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}ConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMinorVersion
        )

# install version config file into datadir/cmake
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}ConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake")


if (TGUY_BUILD_DOCS)
    find_package(Doxygen REQUIRED)

    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doxygen")
    set(DOXYGEN_EXTRACT_STATIC YES)
    doxygen_add_docs(${PROJECT_NAME}_docs
            include src

            ALL
            )
    install(DIRECTORY "${DOXYGEN_OUTPUT_DIRECTORY}" DESTINATION "${CMAKE_INSTALL_DOCDIR}/${PROJECT_NAME}")
endif ()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/install" CACHE PATH "Where to install the library" FORCE)
endif ()
