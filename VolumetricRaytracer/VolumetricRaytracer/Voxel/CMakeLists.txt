# CMakeList.txt : CMake project for VolumetricRaytracer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE voxel_public
	CONFIGURE_DEPENDS
	"Public/*"
)

file(GLOB_RECURSE voxel_private
	CONFIGURE_DEPENDS
	"Private/*"
)

# Add source to this project's executable.
add_library (VVoxel ${voxel_public} ${voxel_private})

target_include_directories(VVoxel PUBLIC "Public")
target_link_libraries(VVoxel VCore)
target_link_libraries(VVoxel VScene)


#target_include_directories(VCore PUBLIC spdlogIncludeDir)

# TODO: Add tests and install targets if needed.
