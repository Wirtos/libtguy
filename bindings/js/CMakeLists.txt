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
    -sEXPORTED_FUNCTIONS=[_malloc,_free,_tguy_from_utf8_ex,_tguy_from_cstr_arr_ex,_tguy_set_frame,_tguy_get_string,_tguy_free,_tguy_get_frames_count,_tguy_get_arr,_tguy_get_version]

    --closure=$<IF:$<CONFIG:Debug>,0,1>
    --post-js "${CMAKE_CURRENT_SOURCE_DIR}/libtguy.js"
)
# trigger target linking if js bindings file changes
set_target_properties(${PROJECT_NAME}_wasm PROPERTIES
    LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libtguy.js"
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