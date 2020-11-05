# CMakeList.txt : CMake project for VolumetricRaytracer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE ui_public
	CONFIGURE_DEPENDS
	"Public/*"
)

file(GLOB_RECURSE ui_private
	CONFIGURE_DEPENDS
	"Private/*"
)

# Add source to this project's executable.


if (WIN32)
	include_directories("Win/Public")

	file(GLOB_RECURSE ui_win_public
		CONFIGURE_DEPENDS
		"Win/Public/*"
	)

	file(GLOB_RECURSE ui_win_private
		CONFIGURE_DEPENDS
		"Win/Private/*"
	)

	if(Boost_FOUND)
		include_directories(${Boost_INCLUDE_DIRS})
	endif()

	add_library (VUI ${ui_public} ${ui_private} ${ui_win_public} ${ui_win_private})
	target_include_directories(VUI PUBLIC "Public")
	target_link_libraries(VUI VCore)

endif (WIN32)

# TODO: Add tests and install targets if needed.