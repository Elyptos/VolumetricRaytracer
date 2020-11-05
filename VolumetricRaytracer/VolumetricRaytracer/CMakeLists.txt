# CMakeList.txt : CMake project for VolumetricRaytracer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

add_subdirectory("Core")
add_subdirectory("UI")
add_subdirectory("Engine")

file(GLOB_RECURSE app_public
	CONFIGURE_DEPENDS
	"App/Public/*"
)

file(GLOB_RECURSE app_private
	CONFIGURE_DEPENDS
	"App/Private/*"
)

# Add source to this project's executable.

if(WIN32)
# "UI/Win/Resources/WindowResources.rct"
add_executable(VolumeRaytracer WIN32 VolumeRaytracer.cpp ${app_private} ${app_public} "UI/Win/Resources/WindowResources.rc")
target_link_libraries(VolumeRaytracer VUI)
endif(WIN32)

target_link_libraries(VolumeRaytracer VEngine)

# TODO: Add tests and install targets if needed.
