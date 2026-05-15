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
option(TGUY_BUILD_DOCS "Build doxygen docs" OFF)
option(TGUY_USE_UTF8PROC "Use utf8proc library for full unicode support. Legacy, use options available in TGUY_UNICODE_LIBRARY instead" OFF)
set(TGUY_UNICODE_LIBRARY "utf8proc" CACHE STRING
    "Select a unicode support backend")
set_property(CACHE TGUY_UNICODE_LIBRARY PROPERTY STRINGS
    "utf8proc" #[["libgrapheme" not yet supported]] "none")
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
    PUBLIC
    $<$<BOOL:${BUILD_SHARED_LIBS}>:LIBTGUY_SHARED_DEFINE>
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
        SOVERSION ${TGUY_VERSION_MAJOR}.${TGUY_VERSION_MINOR}
)

if (TGUY_USE_FASTCLEAR)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TGUY_FASTCLEAR)
endif ()

if ("${TGUY_UNICODE_LIBRARY}" STREQUAL "utf8proc")
    message(STATUS "using utf8proc for unicode support")
    find_package(UTF8Proc CONFIG QUIET)
    if (NOT UTF8Proc_FOUND)
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
    endif()

    target_compile_definitions(${PROJECT_NAME} PRIVATE TGUY_USE_LIBGRAPHEME)
    target_link_libraries(${PROJECT_NAME} PRIVATE grapheme)
endif ()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# install targets' resulting files
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PACKAGE_NAME}Targets
    COMPONENT TGuy
    FILE_SET HEADERS
)

install(
    EXPORT ${PACKAGE_NAME}Targets
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PACKAGE_NAME}"
    NAMESPACE ${PACKAGE_NAME}::
)

# Generate main config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PACKAGE_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PACKAGE_NAME}"
)

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

if (EMSCRIPTEN)
    option(TGUY_WASM_ONLY_BINDINGS "Only include methods which are used by the bindings" OFF)
    if (NOT TGUY_WASM_ONLY_BINDINGS)
        # export all the functions in libtguy and not those explicitly defined in EXPORTED_FUNCTIONS
        target_compile_definitions(${PROJECT_NAME} PRIVATE LIBTGUY_EXPORT=__attribute__\(\(used\)\))
    endif ()
    # cmake emits EXPORT as __attribute__((visibility ("default"))), which doesn't actually keep the function alive in emscripten
    # this is ugly but emscripten toolchain is dumb and doesn't have an option to build library target as wasm
    # make a dummy file as executable needs at least one source to be present
    set(dummy ${CMAKE_CURRENT_BINARY_DIR}/emsdkdummy.c)
    file(WRITE "${dummy}" "")
    add_executable(${PROJECT_NAME}_wasm "${dummy}")
    # for some fucking reason emscripten ignores entire object inside the archive despite being marked as used
    target_link_libraries(${PROJECT_NAME}_wasm PRIVATE ${PROJECT_NAME})

    target_link_options(${PROJECT_NAME}_wasm PRIVATE
        -Wclosure
        -sFILESYSTEM=0
        -sTEXTDECODER=2 # supported pretty much on every browser for ages, we don't need a fallback
        -sMALLOC=emmalloc
        -sENVIRONMENT=web

        -sMODULARIZE=1
        -sEXPORT_NAME=TGuyModule
        -sEXPORTED_RUNTIME_METHODS=[cwrap,lengthBytesUTF8,stringToUTF8,UTF8ToString,setValue,getValue,POINTER_SIZE]
        -sEXPORTED_FUNCTIONS=[_malloc,_free,_tguy_from_utf8_ex,_tguy_from_cstr_arr_ex,_tguy_set_frame,_tguy_get_string,_tguy_free,_tguy_get_frames_count,_tguy_get_arr]

        --closure=$<IF:$<CONFIG:Debug>,0,1>
        --post-js "${CMAKE_CURRENT_SOURCE_DIR}/bindings/libtguy.js"
    )
    #[[ trigger target linking if js bindings file changes ]]
    set_target_properties(${PROJECT_NAME}_wasm PROPERTIES
        LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/bindings/libtguy.js"
    )

    set(TGUY_WASM_BASE_NAME $<TARGET_FILE_PREFIX:${PROJECT_NAME}>$<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>)
    set_target_properties(${PROJECT_NAME}_wasm PROPERTIES OUTPUT_NAME ${TGUY_WASM_BASE_NAME})
    # cmake (or emscripten toolchain) is dumb and doesn't know executable can emit few files so install them manually
    install(
        TARGETS ${PROJECT_NAME}_wasm
        COMPONENT wasm
    )
    install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/${TGUY_WASM_BASE_NAME}.wasm"
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT wasm
    )
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