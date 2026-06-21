cmake_minimum_required(VERSION 3.20)

set(TARGET_NAME CompileShaders)

project(${TARGET_NAME} LANGUAGES C CXX)

set(SOURCES
	main.cpp
)

add_executable(${TARGET_NAME} ${SOURCES})

set_target_properties(${TARGET_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

find_package(slang CONFIG REQUIRED)
target_link_libraries(${TARGET_NAME} PRIVATE slang::slang)

target_link_libraries(${TARGET_NAME} PRIVATE
	SharedLib
	BMEngine_Include
	BmRender
)
