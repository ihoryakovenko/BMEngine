cmake_minimum_required(VERSION 3.20)

set(TARGET_NAME BmRender)

project(${TARGET_NAME} LANGUAGES C CXX)

set(SOURCES
    RenderInterface.h
    Sources/RenderInterface.cpp
    Sources/RenderInternal.h
    VulkanBackend/VulkanCoreContext.cpp
    VulkanBackend/VulkanCoreContext.h
    VulkanBackend/VulkanHelper.cpp
    VulkanBackend/VulkanHelper.h
    VulkanBackend/VulkanRender.cpp
    VulkanBackend/VulkanRender.h
)

add_library(${TARGET_NAME} STATIC ${SOURCES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

target_include_directories(${TARGET_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/Sources   
)

# Disable C++ exceptions and RTTI
if(MSVC)
	target_compile_options(${TARGET_NAME} PRIVATE /EHs-c- /GR-)
    target_compile_definitions(${TARGET_NAME} PRIVATE _HAS_EXCEPTIONS=0)
endif()

set_target_properties(${TARGET_NAME} PROPERTIES
	CXX_EXTENSIONS NO
)

find_package(Vulkan REQUIRED)
target_link_libraries(BmRender PUBLIC
	Vulkan::Vulkan
)

target_link_libraries(${TARGET_NAME} PRIVATE
	SharedLib
)

target_compile_definitions(${TARGET_NAME} PUBLIC
	BM_RENDER_VULKAN_BACKEND
)
