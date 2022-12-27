cmake_minimum_required(VERSION 3.8)

# Some cuda stuff
set(CMAKE_CUDA_ARCHITECTURES 86)

# Project
project(NRC-HPM-Renderer CXX CUDA)

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

file(GLOB_RECURSE PROJECT_INCLUDE "include/*.hpp")
file(GLOB_RECURSE PROJECT_SOURCE "src/*.cpp")
file(GLOB_RECURSE PROJECT_CUDA_SOURCE "src/*.cu")

add_executable(${PROJECT_NAME} ${PROJECT_INCLUDE} ${PROJECT_SOURCE} ${PROJECT_CUDA_SOURCE})
target_include_directories(${PROJECT_NAME} PUBLIC "include" ${CUDA_INC_PATH})

# Compile
list(APPEND CUDA_NVCC_FLAGS "--extended-lambda")
list(APPEND CUDA_NVCC_FLAGS "--expt-relaxed-constexpr")

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_features(${PROJECT_NAME} PUBLIC cuda_std_17)
target_compile_options(${PROJECT_NAME} PUBLIC $<$<COMPILE_LANGUAGE:CUDA>:${CUDA_NVCC_FLAGS}>)

# Dependencies

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)

# CUDA
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})

# GLFW
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

# GLM
find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

# STB
find_path(STB_INCLUDE_DIRS "stb_c_lexer.h")
target_include_directories(${PROJECT_NAME} PRIVATE ${STB_INCLUDE_DIRS})

# IMGUI
find_package(imgui CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)

# TINYEXR
find_package(tinyexr CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::tinyexr::tinyexr)

# ASSIMP
find_package(assimp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)

# TCNN
add_subdirectory("dependencies/tiny-cuda-nn")
target_include_directories(${PROJECT_NAME} PRIVATE "dependencies/tiny-cuda-nn/include" "dependencies/tiny-cuda-nn/dependencies")
target_link_libraries(${PROJECT_NAME} PUBLIC ${CUDA_LIBRARIES} cuda cublas tiny-cuda-nn)

# TBB
find_package(TBB CONFIG REQUIRED)
if (MSVC)
	target_link_libraries(${PROJECT_NAME} PRIVATE TBB::tbb TBB::tbbmalloc TBB::tbbmalloc_proxy)
elseif (UNIX)
	target_link_libraries(${PROJECT_NAME} PRIVATE TBB::tbb TBB::tbbmalloc)
endif()

# OPENVDB
if (MSVC)
	target_include_directories(${PROJECT_NAME} PRIVATE "C:/Program Files/OpenVDB/include")
	find_package(OpenVDB CONFIG REQUIRED)
	target_link_libraries(${PROJECT_NAME} PRIVATE "C:/Program Files/OpenVDB/lib/openvdb.lib")
elseif (UNIX)
	target_include_directories(${PROJECT_NAME} PRIVATE "openvdb-linux/include")
	#target_link_libraries(${PROJECT_NAME} PRIVATE "openvdb-linux/libopenvdb.a")
	target_link_libraries(${PROJECT_NAME} PRIVATE "/p/home/jusers/spindler1/juwels/openvdb/build/openvdb/openvdb/libopenvdb.so.10.0.2")
endif()
