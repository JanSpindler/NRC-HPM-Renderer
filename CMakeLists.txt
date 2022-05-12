cmake_minimum_required(VERSION 3.8)

# Get VCPKG
if (DEFINED ENV{VCPKG_CMAKE_INCLUDE})
	include($ENV{VCPKG_CMAKE_INCLUDE})
endif()

# Project
project(NRC-HPM-Renderer)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE PROJECT_INCLUDE "include/*.hpp")
file(GLOB_RECURSE PROJECT_SOURCE "src/*.cpp")

add_executable(${PROJECT_NAME} ${PROJECT_INCLUDE} ${PROJECT_SOURCE})
target_include_directories(${PROJECT_NAME} PUBLIC "include")

# Dependencies

# It is necessary to pass the DEBUG or RELEASE flag accordingly to Kompute
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1 ${KOMPUTE_EXTRA_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DRELEASE=1 ${KOMPUTE_EXTRA_CXX_FLAGS}")

# Kompute
add_subdirectory("external/kompute")
target_link_libraries(${PROJECT_NAME} PRIVATE kompute::kompute)
include_directories("external/kompute/single_include")

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

# GLFW
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# GLM
find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

# STB
find_path(STB_INCLUDE_DIRS "stb.h")
target_include_directories(${PROJECT_NAME} PRIVATE ${STB_INCLUDE_DIRS})

# IMGUI
find_package(imgui CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)
