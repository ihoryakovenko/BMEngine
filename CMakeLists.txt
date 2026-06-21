cmake_minimum_required(VERSION 3.20)

set(TARGET_NAME BmEngine)
project(${TARGET_NAME} LANGUAGES C CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/x64)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/x64)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/x64)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/BMEngine/Source/*.c"
    "${CMAKE_SOURCE_DIR}/BMEngine/Source/*.cpp"
    "${CMAKE_SOURCE_DIR}/BMEngine/Source/*.h"
    "${CMAKE_SOURCE_DIR}/External/mini-yaml/yaml/Yaml.hpp"
    "${CMAKE_SOURCE_DIR}/External/mini-yaml/yaml/Yaml.cpp"
)

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/BMEngine/Source/Engine/Systems/Render/Shaders/*.slang"
    "${CMAKE_SOURCE_DIR}/BMEngine/Source/Engine/Systems/Render/Shaders/*.glsl"
)

source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${SOURCES})
source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${SHADER_FILES})

add_executable(${TARGET_NAME}
	${SOURCES}
)

target_sources(${TARGET_NAME} PRIVATE 
    ${SOURCES}
    ${SHADER_FILES}
)

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
add_subdirectory(${CMAKE_SOURCE_DIR}/External/glfw)

set(TOOLS
    CompileShaders
    Converter
)

foreach(TOOL ${TOOLS})
    set_target_properties(${TOOL} PROPERTIES
        FOLDER "Tools"
    )
endforeach()

target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/BMEngine/Source
		${CMAKE_SOURCE_DIR}/External
)

find_package(slang CONFIG REQUIRED)

target_link_libraries(${TARGET_NAME} PRIVATE
	slang::slang
	SharedLib
	BmRender
	glfw
)

add_library(BMEngine_Include INTERFACE)
target_include_directories(BMEngine_Include INTERFACE
    ${CMAKE_SOURCE_DIR}/BMEngine/Source
)

add_custom_target(CompileShadersCommand
    COMMAND $<TARGET_FILE:CompileShaders>
        "${CMAKE_SOURCE_DIR}/BMEngine/Source/Engine/Systems/Render/Shaders"
        "${CMAKE_SOURCE_DIR}/BMEngine/Resources/Shaders"
        "${CMAKE_SOURCE_DIR}/BMEngine/Source/Engine/Generated"
    COMMENT "Compiling shaders"
)

add_dependencies(${TARGET_NAME} CompileShadersCommand)

add_custom_command(
    TARGET ${TARGET_NAME}
    POST_BUILD
    COMMAND cmd /c "${CMAKE_SOURCE_DIR}/PostBuildScript.bat"
        "${CMAKE_SOURCE_DIR}"
        "$<TARGET_FILE_DIR:${TARGET_NAME}>"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Copying resources"
)

target_compile_options(${TARGET_NAME} PRIVATE /MP)
