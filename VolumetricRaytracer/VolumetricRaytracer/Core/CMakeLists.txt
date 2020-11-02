# CMakeList.txt : CMake project for VolumetricRaytracer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE core_public
	CONFIGURE_DEPENDS
	"Public/*"
)

file(GLOB_RECURSE core_private
	CONFIGURE_DEPENDS
	"Private/*"
)

# Add source to this project's executable.
add_library (VCore ${core_public} ${core_private})
target_include_directories(VCore PUBLIC "Public")

# TODO: Add tests and install targets if needed.
