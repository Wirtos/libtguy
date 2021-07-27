function(get_version_from_macro)
    set(oneValueArgs OUTPUT PREFIX HEADER)
    set(multiValueArgs VERSIONS)
    cmake_parse_arguments(
            "LOCAL"
            ""
            "${oneValueArgs}"
            "${multiValueArgs}"
            ${ARGN})

    if(NOT DEFINED LOCAL_OUTPUT)
        message(FATAL_ERROR "OUTPUT not defined")
    endif()

    if (NOT DEFINED LOCAL_PREFIX)
        message(FATAL_ERROR "PREFIX not defined")
    endif ()

    if (NOT DEFINED LOCAL_HEADER)
        message(FATAL_ERROR "HEADER not defined")
    endif ()

    if (NOT DEFINED LOCAL_VERSIONS)
        message(STATUS "VERSIONS not provided, using default: MAJOR MINOR PATCH")
        set(LOCAL_VERSIONS MAJOR MINOR PATCH)
    endif ()

    set(HEADER_VER)
    foreach(version IN ITEMS ${LOCAL_VERSIONS})
        set(macrostr "#define ${LOCAL_PREFIX}${version}")
        file(STRINGS "${LOCAL_HEADER}" version REGEX "${macrostr}")
        string(REGEX REPLACE "${macrostr}[ \\t]+([0-9]+)" "" version "${version}")
        if ("${CMAKE_MATCH_1}" STREQUAL "")
            message(FATAL_ERROR "macro \"${macrostr} <num>\" not found")
        endif ()
        list(APPEND HEADER_VER "${CMAKE_MATCH_1}")
    endforeach()

    string(JOIN "." HEADER_VER ${HEADER_VER})
    set(${LOCAL_OUTPUT} ${HEADER_VER} PARENT_SCOPE)
endfunction()
