# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#

cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE vox_public
	CONFIGURE_DEPENDS
	"Public/*"
)

file(GLOB_RECURSE vox_private
	CONFIGURE_DEPENDS
	"Private/*"
)

add_executable(Voxelizer ${vox_public} ${vox_private})
add_dependencies (Voxelizer gltf)

target_include_directories(Voxelizer PUBLIC "Public")

if(Boost_FOUND)
	target_include_directories(Voxelizer PUBLIC ${Boost_INCLUDE_DIRS})
	target_link_libraries(Voxelizer ${Boost_LIBRARIES})
endif()

target_link_directories(Voxelizer PUBLIC ${gltfLibDir})
target_include_directories(Voxelizer PUBLIC ${gltfIncludeDir})

target_link_libraries(Voxelizer VScene)
target_link_libraries(Voxelizer VVoxel)
target_link_libraries(Voxelizer GLTFSDK)