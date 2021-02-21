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

	find_program(hlslCompiler dxc)

	if(hlslCompiler)
		message("HLSL shader compiler found at path " ${hlslCompiler})
		message("Compiling shaders")

		add_custom_command(
			OUTPUT "DX/Resources/Shaders/Compiled/Raytracing.hlsl.h"
			COMMAND ${hlslCompiler} -Zi -Od -Vn "g_pRaytracing" -Fo "./DX/Resources/Shaders/Compiled/Raytracing.cso" -T "lib_6_3"  -Fh "./DX/Resources/Shaders/Compiled/Raytracing.hlsl.h" "./DX/Resources/Shaders/Raytracing.hlsl"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS "DX/Resources/Shaders/RaytracingHlsl.h" "DX/Resources/Shaders/Raytracing.hlsl" "DX/Resources/Shaders/Include/Constants.hlsli" "DX/Resources/Shaders/Include/Debugging.hlsli" "DX/Resources/Shaders/Include/Lighting.hlsli" "DX/Resources/Shaders/Include/Parameters.hlsli" "DX/Resources/Shaders/Include/Quaternion.hlsli" "DX/Resources/Shaders/Include/Ray.hlsli" "DX/Resources/Shaders/Include/Textures.hlsli" "DX/Resources/Shaders/Include/Voxel.hlsli"
		)

		add_custom_command(
			OUTPUT "DX/Resources/Shaders/Compiled/Raytracing_Unlit.hlsl.h"
			COMMAND ${hlslCompiler} -Zi -Od -Vn "g_pRaytracing_unlit" -Fo "./DX/Resources/Shaders/Compiled/Raytracing_Unlit.cso" -T "lib_6_3"  -Fh "./DX/Resources/Shaders/Compiled/Raytracing_Unlit.hlsl.h" "./DX/Resources/Shaders/Raytracing_Unlit.hlsl"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS "DX/Resources/Shaders/RaytracingHlsl.h" "DX/Resources/Shaders/Raytracing_Unlit.hlsl" "DX/Resources/Shaders/Include/Constants.hlsli" "DX/Resources/Shaders/Include/Debugging.hlsli" "DX/Resources/Shaders/Include/Lighting.hlsli" "DX/Resources/Shaders/Include/Parameters.hlsli" "DX/Resources/Shaders/Include/Quaternion.hlsli" "DX/Resources/Shaders/Include/Ray.hlsli" "DX/Resources/Shaders/Include/Textures.hlsli" "DX/Resources/Shaders/Include/Voxel.hlsli"
		)

		add_custom_command(
			OUTPUT "DX/Resources/Shaders/Compiled/Raytracing_NoTex.hlsl.h"
			COMMAND ${hlslCompiler} -Zi -Od -Vn "g_pRaytracing_noTex" -Fo "./DX/Resources/Shaders/Compiled/Raytracing_NoTex.cso" -T "lib_6_3"  -Fh "./DX/Resources/Shaders/Compiled/Raytracing_NoTex.hlsl.h" "./DX/Resources/Shaders/Raytracing_NoTex.hlsl"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS "DX/Resources/Shaders/RaytracingHlsl.h" "DX/Resources/Shaders/Raytracing_NoTex.hlsl" "DX/Resources/Shaders/Include/Constants.hlsli" "DX/Resources/Shaders/Include/Debugging.hlsli" "DX/Resources/Shaders/Include/Lighting.hlsli" "DX/Resources/Shaders/Include/Parameters.hlsli" "DX/Resources/Shaders/Include/Quaternion.hlsli" "DX/Resources/Shaders/Include/Ray.hlsli" "DX/Resources/Shaders/Include/Textures.hlsli" "DX/Resources/Shaders/Include/Voxel.hlsli"
		)

		add_custom_command(
			OUTPUT "DX/Resources/Shaders/Compiled/Raytracing_NoTex_Unlit.hlsl.h"
			COMMAND ${hlslCompiler} -Zi -Od -Vn "g_pRaytracing_noTex_unlit" -Fo "./DX/Resources/Shaders/Compiled/Raytracing_NoTex_Unlit.cso" -T "lib_6_3"  -Fh "./DX/Resources/Shaders/Compiled/Raytracing_NoTex_Unlit.hlsl.h" "./DX/Resources/Shaders/Raytracing_NoTex_Unlit.hlsl"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS "DX/Resources/Shaders/RaytracingHlsl.h" "DX/Resources/Shaders/Raytracing_NoTex_Unlit.hlsl" "DX/Resources/Shaders/Include/Constants.hlsli" "DX/Resources/Shaders/Include/Debugging.hlsli" "DX/Resources/Shaders/Include/Lighting.hlsli" "DX/Resources/Shaders/Include/Parameters.hlsli" "DX/Resources/Shaders/Include/Quaternion.hlsli" "DX/Resources/Shaders/Include/Ray.hlsli" "DX/Resources/Shaders/Include/Textures.hlsli" "DX/Resources/Shaders/Include/Voxel.hlsli"
		)

		add_custom_command(
			OUTPUT "DX/Resources/Shaders/Compiled/Raytracing_Cube.hlsl.h"
			COMMAND ${hlslCompiler} -Zi -Od -Vn "g_pRaytracing_cube" -Fo "./DX/Resources/Shaders/Compiled/Raytracing_Cube.cso" -T "lib_6_3"  -Fh "./DX/Resources/Shaders/Compiled/Raytracing_Cube.hlsl.h" "./DX/Resources/Shaders/Raytracing_Cube.hlsl"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS "DX/Resources/Shaders/RaytracingHlsl.h" "DX/Resources/Shaders/Raytracing_Cube.hlsl" "DX/Resources/Shaders/Include/Constants.hlsli" "DX/Resources/Shaders/Include/Debugging.hlsli" "DX/Resources/Shaders/Include/Lighting.hlsli" "DX/Resources/Shaders/Include/Parameters.hlsli" "DX/Resources/Shaders/Include/Quaternion.hlsli" "DX/Resources/Shaders/Include/Ray.hlsli" "DX/Resources/Shaders/Include/Textures.hlsli" "DX/Resources/Shaders/Include/Voxel.hlsli"
		)

		add_custom_command(
			OUTPUT "DX/Resources/Shaders/Compiled/Raytracing_Cube_NoTex.hlsl.h"
			COMMAND ${hlslCompiler} -Zi -Od -Vn "g_pRaytracing_cube_noTex" -Fo "./DX/Resources/Shaders/Compiled/Raytracing_Cube_NoTex.cso" -T "lib_6_3"  -Fh "./DX/Resources/Shaders/Compiled/Raytracing_Cube_NoTex.hlsl.h" "./DX/Resources/Shaders/Raytracing_Cube_NoTex.hlsl"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS "DX/Resources/Shaders/RaytracingHlsl.h" "DX/Resources/Shaders/Raytracing_Cube_NoTex.hlsl" "DX/Resources/Shaders/Include/Constants.hlsli" "DX/Resources/Shaders/Include/Debugging.hlsli" "DX/Resources/Shaders/Include/Lighting.hlsli" "DX/Resources/Shaders/Include/Parameters.hlsli" "DX/Resources/Shaders/Include/Quaternion.hlsli" "DX/Resources/Shaders/Include/Ray.hlsli" "DX/Resources/Shaders/Include/Textures.hlsli" "DX/Resources/Shaders/Include/Voxel.hlsli"
		)

		add_custom_command(
			OUTPUT "DX/Resources/Shaders/Compiled/Raytracing_Cube_NoTex_Unlit.hlsl.h"
			COMMAND ${hlslCompiler} -Zi -Od -Vn "g_pRaytracing_cube_noTex_unlit" -Fo "./DX/Resources/Shaders/Compiled/Raytracing_Cube_NoTex_Unlit.cso" -T "lib_6_3"  -Fh "./DX/Resources/Shaders/Compiled/Raytracing_Cube_NoTex_Unlit.hlsl.h" "./DX/Resources/Shaders/Raytracing_Cube_NoTex_Unlit.hlsl"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS "DX/Resources/Shaders/RaytracingHlsl.h" "DX/Resources/Shaders/Raytracing_Cube_NoTex_Unlit.hlsl" "DX/Resources/Shaders/Include/Constants.hlsli" "DX/Resources/Shaders/Include/Debugging.hlsli" "DX/Resources/Shaders/Include/Lighting.hlsli" "DX/Resources/Shaders/Include/Parameters.hlsli" "DX/Resources/Shaders/Include/Quaternion.hlsli" "DX/Resources/Shaders/Include/Ray.hlsli" "DX/Resources/Shaders/Include/Textures.hlsli" "DX/Resources/Shaders/Include/Voxel.hlsli"
		)

		add_custom_command(
			OUTPUT "DX/Resources/Shaders/Compiled/Raytracing_Cube_Unlit.hlsl.h"
			COMMAND ${hlslCompiler} -Zi -Od -Vn "g_pRaytracing_cube_unlit" -Fo "./DX/Resources/Shaders/Compiled/Raytracing_Cube_Unlit.cso" -T "lib_6_3"  -Fh "./DX/Resources/Shaders/Compiled/Raytracing_Cube_Unlit.hlsl.h" "./DX/Resources/Shaders/Raytracing_Cube_Unlit.hlsl"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS "DX/Resources/Shaders/RaytracingHlsl.h" "DX/Resources/Shaders/Raytracing_Cube_Unlit.hlsl" "DX/Resources/Shaders/Include/Constants.hlsli" "DX/Resources/Shaders/Include/Debugging.hlsli" "DX/Resources/Shaders/Include/Lighting.hlsli" "DX/Resources/Shaders/Include/Parameters.hlsli" "DX/Resources/Shaders/Include/Quaternion.hlsli" "DX/Resources/Shaders/Include/Ray.hlsli" "DX/Resources/Shaders/Include/Textures.hlsli" "DX/Resources/Shaders/Include/Voxel.hlsli"
		)

		#execute_process(
		#	COMMAND ${hlslCompiler} -Zi -Od -Vn "g_pRaytracing" -Fo "./DX/Resources/Shaders/Compiled/Raytracing.cso" -T "lib_6_3"  -Fh "./DX/Resources/Shaders/Compiled/Raytracing.hlsl.h" "./DX/Resources/Shaders/Raytracing.hlsl"
		#	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		#	RESULT_VARIABLE hlslCompilerResult
		#)
	else()
		message(WARNING "HLSL shader compiler not found")
	endif()

	# Add source to this project's executable.

	#include_directories(${d3dx12IncludeDir})

	set(dx_shader_include "DX/Resources/Shaders/RaytracingHlsl.h" "DX/Resources/Shaders/Compiled/Raytracing.hlsl.h" "DX/Resources/Shaders/Compiled/Raytracing_Unlit.hlsl.h" "DX/Resources/Shaders/Compiled/Raytracing_NoTex.hlsl.h" "DX/Resources/Shaders/Compiled/Raytracing_NoTex_Unlit.hlsl.h" "DX/Resources/Shaders/Compiled/Raytracing_Cube.hlsl.h" "DX/Resources/Shaders/Compiled/Raytracing_Cube_Unlit.hlsl.h" "DX/Resources/Shaders/Compiled/Raytracing_Cube_NoTex.hlsl.h" "DX/Resources/Shaders/Compiled/Raytracing_Cube_NoTex_Unlit.hlsl.h")

	add_library (VRenderer ${renderer_public} ${renderer_private} ${renderer_dx_public} ${renderer_dx_private} ${dx_shader_include})

	if(Boost_FOUND)
		target_include_directories(VRenderer PUBLIC ${Boost_INCLUDE_DIRS})
	endif()

	target_link_libraries(VRenderer VCore)
	target_link_libraries(VRenderer VVoxel)
	target_link_libraries(VRenderer VScene)
	target_link_libraries(VRenderer ${Boost_LIBRARIES})
	target_link_libraries(VRenderer "d3d12.lib" "dxgi.lib" "d3dcompiler.lib" "dxguid.lib")
	target_link_libraries(VRenderer Ext_DirectXTex)

	target_include_directories(VRenderer PUBLIC "Public")
	target_include_directories(VRenderer PUBLIC "DX/Public")
	target_include_directories(VRenderer PUBLIC "DX/Resources/Shaders")
	target_include_directories(VRenderer PUBLIC ${d3dx12IncludeDir})
	#target_include_directories(VRenderer PUBLIC ${DirectXTex_INCLUDE_DIRS})
endif(WIN32)