cmake_minimum_required(VERSION 3.10)
project(tguy VERSION 0.0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(TGuy_PACKAGE TGuy)

add_library(${PROJECT_NAME} src/libtguy.c)
add_library(${TGuy_PACKAGE}::TGuy ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
# export tguy target as TGuy
# declare headers to be installed
set_target_properties(${PROJECT_NAME} PROPERTIES
        EXPORT_NAME TGuy
        PUBLIC_HEADER "include/libtguy.h")


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