# CMakeList.txt : CMake project for VolumetricRaytracer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

add_subdirectory("Core")
add_subdirectory("UI")

# Add source to this project's executable.

if(WIN32)
# "UI/Win/Resources/WindowResources.rct"
add_executable(VolumeRaytracer WIN32 VolumeRaytracer.cpp "UI/Win/Resources/WindowResources.rc")
target_link_libraries(VolumeRaytracer VUI)
endif(WIN32)
# TODO: Add tests and install targets if needed.
