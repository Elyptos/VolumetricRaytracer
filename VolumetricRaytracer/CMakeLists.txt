# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)
include(ExternalProject)
find_package(Git REQUIRED)

project ("VolumetricRaytracer")

if (POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# External git repos
ExternalProject_Add(spdlog
	PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/ext/spdlog
	GIT_REPOSITORY https://github.com/gabime/spdlog.git
	GIT_TAG v1.8.1
	GIT_PROGRESS 1
	UPDATE_COMMAND ""
	INSTALL_COMMAND ""
	BUILD_COMMAND ""
	LOG_DOWNLOAD ON
)

ExternalProject_Add(Eigen3
	PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/ext/eigen3
	URL https://gitlab.com/libeigen/eigen/-/archive/3.3.8/eigen-3.3.8.tar.gz
	UPDATE_COMMAND ""
	CONFIGURE_COMMAND ""
	INSTALL_COMMAND ""
	BUILD_COMMAND ""
	LOG_DOWNLOAD ON
)

ExternalProject_Add(rapidjson
	PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/ext/rapidjson
	GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
	GIT_TAG v1.1.0
	GIT_PROGRESS 1
	UPDATE_COMMAND ""
	INSTALL_COMMAND ""
	BUILD_COMMAND ""
	LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(spdlog source_dir)
set(spdlogIncludeDir ${source_dir}/include)

ExternalProject_Get_Property(Eigen3 source_dir)
set(eigenIncludeDir ${source_dir})

ExternalProject_Get_Property(rapidjson source_dir)
set(rapidjsonIncludeDir ${source_dir}/include)

#gltf
ExternalProject_Add(gltf
	PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/ext/gltf
	GIT_REPOSITORY https://github.com/microsoft/glTF-SDK.git
	GIT_TAG r1.9.5.0
	GIT_PROGRESS 1
	LOG_DOWNLOAD ON
	INSTALL_COMMAND ""
)

ExternalProject_Get_Property(gltf source_dir)
set(gltfIncludeDir ${source_dir}/GLTFSDK/Inc)

ExternalProject_Get_Property(gltf binary_dir)
set(gltfLibDir ${binary_dir}/GLTFSDK/)

#file(DOWNLOAD https://raw.githubusercontent.com/microsoft/DirectX-Graphics-Samples/v1.5-dxr/Libraries/D3DX12/d3dx12.h ${CMAKE_CURRENT_SOURCE_DIR}/ext/d3dx12/include/d3dx12.h)
file(DOWNLOAD https://raw.githubusercontent.com/microsoft/DirectX-Graphics-Samples/225977/Libraries/D3DX12/d3dx12.h ${CMAKE_CURRENT_SOURCE_DIR}/ext/d3dx12/include/d3dx12.h)
set(d3dx12IncludeDir ${CMAKE_CURRENT_SOURCE_DIR}/ext/d3dx12/include)

# Boost
set(Boost_DEBUG 1)
set(Boost_USE_STATIC_LIBS   ON)

set(BOOST_INCLUDEDIR $ENV{BOOST_ROOT}/boost)
set(BOOST_LIBRARYDIR $ENV{BOOST_ROOT}/stage/lib)

find_package(Boost 1.74 REQUIRED COMPONENTS filesystem)

# OpenMP
find_package(OpenMP)

# Include sub-projects.
add_subdirectory("Dependencies/DirectXTex")
add_subdirectory ("VolumetricRaytracer")
add_subdirectory("Voxelizer")