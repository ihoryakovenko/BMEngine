cmake_minimum_required(VERSION 3.20)

set(TARGET_NAME BmEngine)
project(${TARGET_NAME} LANGUAGES C CXX)

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/x64)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/x64)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/x64)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/BMEngine/Source/*.c"
    "${CMAKE_SOURCE_DIR}/BMEngine/Source/*.cpp"
    "${CMAKE_SOURCE_DIR}/BMEngine/Source/*.h"
    "${CMAKE_SOURCE_DIR}/External/mini-yaml/yaml/Yaml.hpp"
    "${CMAKE_SOURCE_DIR}/External/mini-yaml/yaml/Yaml.cpp"
)

add_executable(${TARGET_NAME} ${SOURCES})

set_target_properties(${TARGET_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS OFF
)

if(MSVC)
	set_target_properties(${TARGET_NAME} PROPERTIES
		VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>"
	)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/SharedLib)
add_subdirectory(${CMAKE_SOURCE_DIR}/BmRender)
add_subdirectory(${CMAKE_SOURCE_DIR}/BmEngine/Tools/CompileShaders)
add_subdirectory(${CMAKE_SOURCE_DIR}/BmEngine/Tools/Converter)

target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/BMEngine/Source
		${CMAKE_SOURCE_DIR}/External/glfw/include
		${CMAKE_SOURCE_DIR}/External
)

find_package(slang CONFIG REQUIRED)
target_link_libraries(${TARGET_NAME} PRIVATE slang::slang)

target_link_libraries(${TARGET_NAME} PRIVATE
	SharedLib
	BmRender
	${CMAKE_SOURCE_DIR}/External/glfw/lib-vc2022/glfw3.lib
)

add_library(BMEngine_Include INTERFACE)
target_include_directories(BMEngine_Include INTERFACE
    ${CMAKE_SOURCE_DIR}/BMEngine/Source
)
