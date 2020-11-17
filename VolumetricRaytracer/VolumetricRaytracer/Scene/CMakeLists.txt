# CMakeList.txt : CMake project for VolumetricRaytracer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE scene_public
	CONFIGURE_DEPENDS
	"Public/*"
)

file(GLOB_RECURSE scene_private
	CONFIGURE_DEPENDS
	"Private/*"
)

# Add source to this project's executable.
add_library (VScene ${scene_public} ${scene_private})

target_include_directories(VScene PUBLIC "Public")
target_link_libraries(VScene VCore)


#target_include_directories(VCore PUBLIC spdlogIncludeDir)

# TODO: Add tests and install targets if needed.
