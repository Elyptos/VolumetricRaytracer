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
add_dependencies (VCore spdlog)

add_dependencies (VCore Eigen3)

target_include_directories(VCore PUBLIC "Public")
target_include_directories(VCore PUBLIC ${eigenIncludeDir})
include_directories(${spdlogIncludeDir})

if(Boost_FOUND)
	target_include_directories(VCore PUBLIC ${Boost_INCLUDE_DIRS})
endif()

target_link_libraries(VCore ${Boost_LIBRARIES})

#target_include_directories(VCore PUBLIC spdlogIncludeDir)

# TODO: Add tests and install targets if needed.
