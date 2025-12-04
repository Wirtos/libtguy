#[[
This function looks for version number macros and sets them as cmake variables
Example :
get_version_from_macro(
    OUTPUT TGUY_VERSION
    HEADER libtguy.h
    PREFIX TGUY_VER_
    VERSIONS MAJOR MINOR PATCH
    )

This will result in function searching HEADER file for such macros:
#define TGUY_VER_MAJOR 0
#define TGUY_VER_MINOR 1
#define TGUY_VER_PATCH 2

After the function returns, variables prefixed as OUTPUT will be set:
TGUY_VERSION = "0.1.2"
TGUY_VERSION_MAJOR = "0"
TGUY_VERSION_MINOR = "1"
TGUY_VERSION_PATCH = "2"
]]
function(get_version_from_macro)
    set(options)
    set(oneValueArgs OUTPUT PREFIX HEADER)
    set(multiValueArgs VERSIONS)
    cmake_parse_arguments(_args
            "${options}"
            "${oneValueArgs}"
            "${multiValueArgs}"
            ${ARGN})

    foreach (arg IN ITEMS OUTPUT PREFIX HEADER)
        if (NOT DEFINED _args_${arg})
            message(FATAL_ERROR "${arg} not provided")
        endif ()
    endforeach ()

    if (NOT DEFINED _args_VERSIONS)
        message(STATUS "VERSIONS not provided, using default: MAJOR MINOR PATCH")
        set(_args_VERSIONS MAJOR MINOR PATCH)
    endif ()

    set(HEADER_VER)
    foreach (version IN ITEMS ${_args_VERSIONS})
        set(version_name ${version})
        # find #define line
        set(macrostr "#define[ \\t]+${_args_PREFIX}${version_name}")
        file(STRINGS "${_args_HEADER}" version REGEX "${macrostr}")
        # match number in the line
        string(REGEX MATCH "${macrostr}[ \\t]+([0-9]+)" version "${version}")
        if ("${CMAKE_MATCH_1}" STREQUAL "")
            message(FATAL_ERROR "\"#define ${_args_PREFIX}${version_name} <num>\" not found in ${_args_HEADER}")
        endif ()
        message(STATUS "Setting ${_args_OUTPUT}_${version_name}")
        set(${_args_OUTPUT}_${version_name} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        list(APPEND HEADER_VER "${CMAKE_MATCH_1}")
    endforeach ()

    string(JOIN "." HEADER_VER ${HEADER_VER})
    set(${_args_OUTPUT} ${HEADER_VER} PARENT_SCOPE)
endfunction()
