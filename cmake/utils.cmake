function(get_version_string header libname out)
    foreach(vspec IN ITEMS MAJOR MINOR PATCH)
        set(macrostr "#define ${libname}_VER_${vspec}")
        file(STRINGS "${header}" vspec REGEX "${macrostr}")
        string(REGEX REPLACE "${macrostr}[ ]+([0-9]+)" "" vspec "${vspec}")
        list(APPEND HEADER_VER "${CMAKE_MATCH_1}")
    endforeach()
    string(JOIN "." HEADER_VER ${HEADER_VER})
    set(${out} ${HEADER_VER} PARENT_SCOPE)
endfunction()
