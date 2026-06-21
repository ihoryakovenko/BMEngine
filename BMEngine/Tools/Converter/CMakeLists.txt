cmake_minimum_required(VERSION 3.20)

set(TARGET_NAME Converter)

project(${TARGET_NAME} LANGUAGES C CXX)

set(SOURCES
	main.cpp
)

add_executable(${TARGET_NAME} ${SOURCES})

set_target_properties(${TARGET_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(${TARGET_NAME} PRIVATE
	SharedLib
	BMEngine_Include
	BmRender
	${CMAKE_SOURCE_DIR}/External/glfw/lib-vc2022/glfw3.lib
)

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/External/glfw/include
    ${CMAKE_SOURCE_DIR}/External
)
