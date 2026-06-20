cmake_minimum_required(VERSION 3.20)

set(TARGET_NAME SharedLib)

project(${TARGET_NAME} LANGUAGES C CXX)

add_library(${TARGET_NAME} STATIC
	SharedLib.cpp
	f_mem_debug.c
	SharedLib.h
	forge_memory_debugger.h
	ShortTypes.h
)

target_include_directories(${TARGET_NAME} PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

# Disable C++ exceptions and RTTI
if(MSVC)
	target_compile_options(${TARGET_NAME} PRIVATE /EHs-c- /GR-)
    target_compile_definitions(${TARGET_NAME} PRIVATE _HAS_EXCEPTIONS=0)
endif()

set_target_properties(${TARGET_NAME} PROPERTIES
	CXX_EXTENSIONS NO
)

