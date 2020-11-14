# CMakeList.txt : CMake project for VolumetricRaytracer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE renderer_public
	CONFIGURE_DEPENDS
	"Public/*"
)

file(GLOB_RECURSE renderer_private
	CONFIGURE_DEPENDS
	"Private/*"
)

if (WIN32)
	file(GLOB_RECURSE renderer_dx_public
		CONFIGURE_DEPENDS
		"DX/Public/*"
	)

	file(GLOB_RECURSE renderer_dx_private
		CONFIGURE_DEPENDS
		"DX/Private/*"
	)

	# Add source to this project's executable.

	#include_directories(${d3dx12IncludeDir})

	set(dx_shader_include "DX/Resources/Shaders/RaytracingHlsl.h")

	add_library (VRenderer ${renderer_public} ${renderer_private} ${renderer_dx_public} ${renderer_dx_private} ${dx_shader_include})

	if(Boost_FOUND)
		target_include_directories(VRenderer PUBLIC ${Boost_INCLUDE_DIRS})
	endif()

	target_link_libraries(VRenderer VCore)
	target_link_libraries(VRenderer ${Boost_LIBRARIES})
	target_link_libraries(VRenderer "d3d12.lib" "dxgi.lib" "d3dcompiler.lib" "dxguid.lib")

	target_include_directories(VRenderer PUBLIC "Public")
	target_include_directories(VRenderer PUBLIC "DX/Public")
	target_include_directories(VRenderer PUBLIC "DX/Resources/Shaders")
	target_include_directories(VRenderer PUBLIC ${d3dx12IncludeDir})
endif(WIN32)