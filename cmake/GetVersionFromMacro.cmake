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
        set(macrostr "#define ${_args_PREFIX}${version}")
        file(STRINGS "${_args_HEADER}" version REGEX "${macrostr}")
        string(REGEX REPLACE "${macrostr}[ \\t]+([0-9]+)" "" version "${version}")
        if ("${CMAKE_MATCH_1}" STREQUAL "")
            message(FATAL_ERROR "macro \"${macrostr} <num>\" not found")
        endif ()
        list(APPEND HEADER_VER "${CMAKE_MATCH_1}")
    endforeach ()

    string(JOIN "." HEADER_VER ${HEADER_VER})
    set(${_args_OUTPUT} ${HEADER_VER} PARENT_SCOPE)
endfunction()
