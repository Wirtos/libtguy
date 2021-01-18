cmake_minimum_required(VERSION 3.10)
project(libtguy VERSION 0.0.2 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(TGUY_USE_FASTCLEAR "Whether to double space usage to increase speed" ON)

set(TGuy_PACKAGE TGuy)

add_library(${PROJECT_NAME} src/libtguy.c)
if("${CMAKE_C_COMPILER_ID}" MATCHES "MSVC" OR ("${CMAKE_C_COMPILER_ID}" MATCHES "Intel|Clang" AND "${CMAKE_C_SIMULATE_ID}" STREQUAL "MSVC"))
    target_compile_options(${PROJECT_NAME}
            PRIVATE
            $<$<CONFIG:Release>:-Ox;-fp:fast;> -W4 -WX)
elseif(${CMAKE_C_COMPILER_ID} MATCHES "GNU|Intel|Clang")
    target_compile_options(${PROJECT_NAME}
            PRIVATE
            $<$<CONFIG:Release>:-Ofast;-fno-math-errno;> $<$<C_COMPILER_ID:GNU,Clang>:-pedantic-errors;> -Wall -Wextra -Werror)
endif()

add_library(${TGuy_PACKAGE}::TGuy ALIAS ${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PUBLIC $<$<BOOL:${TGUY_USE_FASTCLEAR}>:TGUY_FASTCLEAR>)

target_include_directories(${PROJECT_NAME} PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
# export tguy target as TGuy
# declare headers to be installed
set_target_properties(${PROJECT_NAME} PROPERTIES
        EXPORT_NAME TGuy
        PUBLIC_HEADER "include/libtguy.h"
        PREFIX "")

include(CheckLibraryExists)

check_library_exists(m pow "" LIBM_EXISTS)
if (LIBM_EXISTS)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif ()

include(GNUInstallDirs)

install(TARGETS
        ${PROJECT_NAME}
        EXPORT ${TGuy_PACKAGE}Config
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

export(TARGETS
        ${PROJECT_NAME}
        NAMESPACE ${TGuy_PACKAGE}::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/TGuyConfig.cmake"
        )

install(EXPORT
        ${TGuy_PACKAGE}Config
        DESTINATION "${CMAKE_INSTALL_DATADIR}/${TGuy_PACKAGE}/cmake"
        NAMESPACE ${TGuy_PACKAGE}::
        )
