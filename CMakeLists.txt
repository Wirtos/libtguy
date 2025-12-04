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
option(TGUY_USE_FASTCLEAR "Double arena memory usage to increase speed" ON)
option(TGUY_USE_UTF8PROC "Use utf8proc library for full unicode support" ON)
option(TGUY_BUILD_DOCS "Build doxygen docs" OFF)

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
    $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:LIBTGUY_STATIC_DEFINE>
)

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
        SOVERSION ${TGUY_VERSION_MAJOR}
)

if (TGUY_USE_FASTCLEAR)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TGUY_FASTCLEAR)
endif ()

if (TGUY_USE_UTF8PROC)
    find_package(UTF8Proc CONFIG QUIET)
    if (NOT UTF8Proc_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            utf8proc
            GIT_REPOSITORY https://github.com/JuliaStrings/utf8proc
            GIT_TAG 90daf9f396cfec91668758eb9cc54bd5248a6b89
        )
        set(BUILD_SHARED_LIBS OFF)
        FetchContent_MakeAvailable(utf8proc)
        unset(BUILD_SHARED_LIBS)
    endif ()

    target_compile_definitions(${PROJECT_NAME} PRIVATE TGUY_USE_UTF8PROC)
    target_link_libraries(${PROJECT_NAME} PRIVATE utf8proc)
endif ()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# install targets' resulting files
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PACKAGE_NAME}Targets
    COMPONENT TGuy
    FILE_SET HEADERS
)

install(EXPORT ${PACKAGE_NAME}Targets
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PACKAGE_NAME}"
    NAMESPACE {PACKAGE_NAME}::
)

# Generate main config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PACKAGE_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PACKAGE_NAME}"
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}Version.cmake
    COMPATIBILITY SameMajorVersion
)

if (EMSCRIPTEN)
    # cmake emits EXPORT as __attribute__((visibility ("default"))), which doesn't actually keep the function alive in emscripten
    target_compile_definitions(${PROJECT_NAME} PRIVATE LIBTGUY_EXPORT=__attribute__\(\(used\)\))
    # this is ugly but emscripten toolchain is dumb and doesn't have an option to build library target as wasm
    # make a dummy file as executable needs at least one source to be present
    set(dummy ${CMAKE_CURRENT_BINARY_DIR}/emsdkdummy.c)
    file(WRITE "${dummy}" "")
    add_executable(${PROJECT_NAME}_wasm "${dummy}")
    # for some fucking reason emscripten ignores entire object inside the archive despite being marked as used
    target_link_libraries(${PROJECT_NAME}_wasm PRIVATE -Wl,--whole-archive ${PROJECT_NAME} -Wl,--no-whole-archive)

    target_link_options(${PROJECT_NAME}_wasm PRIVATE
        -Wclosure
        -sEXPORTED_RUNTIME_METHODS=[cwrap,lengthBytesUTF8,stringToUTF8]
        -sEXPORTED_FUNCTIONS=[_malloc,_free]
        -sMODULARIZE=1
        -sFILESYSTEM=0
        -sEXPORT_NAME=TGuyModule
        --closure=$<IF:$<CONFIG:Debug>,0,1>
        --extern-post-js "${CMAKE_CURRENT_SOURCE_DIR}/bindings/libtguy.js"
    )

    set(TGUY_WASM_BASE_NAME $<TARGET_FILE_PREFIX:${PROJECT_NAME}>$<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>)
    set_target_properties(${PROJECT_NAME}_wasm PROPERTIES OUTPUT_NAME ${TGUY_WASM_BASE_NAME})
    # cmake (or emscripten toolchain) is dumb and doesn't know executable can emit few files so install them manually
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${TGUY_WASM_BASE_NAME}.wasm"
        "${CMAKE_CURRENT_BINARY_DIR}/${TGUY_WASM_BASE_NAME}.js"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/${PACKAGE_NAME}")
endif ()

if (TGUY_BUILD_DOCS)
    find_package(Doxygen REQUIRED)

    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doxygen")
    set(DOXYGEN_EXTRACT_STATIC YES)
    doxygen_add_docs(${PROJECT_NAME}_docs
        libtguy.c
        libtguy.h
        ALL
    )
    install(DIRECTORY "${DOXYGEN_OUTPUT_DIRECTORY}" DESTINATION "${CMAKE_INSTALL_DOCDIR}/${PROJECT_NAME}")
endif ()