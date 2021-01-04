# CMakeList.txt : CMake project for VolumetricRaytracer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE engine_public
	CONFIGURE_DEPENDS
	"Public/*"
)

file(GLOB_RECURSE engine_private
	CONFIGURE_DEPENDS
	"Private/*"
)

if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
endif()

# Add source to this project's executable.
add_library (VEngine ${engine_public} ${engine_private})
target_link_libraries(VEngine VCore)
target_link_libraries(VEngine ${Boost_LIBRARIES})
target_link_libraries(VEngine VRenderer)

target_include_directories(VEngine PUBLIC "Public")