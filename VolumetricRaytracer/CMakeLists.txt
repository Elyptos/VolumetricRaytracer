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

ExternalProject_Get_Property(spdlog source_dir)
set(spdlogIncludeDir ${source_dir}/include)

# Boost
set(Boost_DEBUG 1)
set(Boost_USE_STATIC_LIBS   ON)

set(BOOST_INCLUDEDIR $ENV{BOOST_ROOT}/boost)
set(BOOST_LIBRARYDIR $ENV{BOOST_ROOT}/stage/lib)

find_package(Boost 1.74 REQUIRED COMPONENTS filesystem)

# Include sub-projects.
add_subdirectory ("VolumetricRaytracer")