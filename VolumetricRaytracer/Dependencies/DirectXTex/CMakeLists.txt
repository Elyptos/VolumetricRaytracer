# CMakeList.txt : CMake project for VolumetricRaytracer, include source and define
# project specific logic here.
#

cmake_minimum_required (VERSION 3.8)

file(GLOB_RECURSE dxtex_files
	CONFIGURE_DEPENDS
	"DirectXTex/*"
)

file(GLOB_RECURSE scene_private
	CONFIGURE_DEPENDS
	"Private/*"
)

add_library (Ext_DirectXTex "DDSTextureLoader/DDSTextureLoader12.h" "DDSTextureLoader/DDSTextureLoader12.cpp" 
	"WICTextureLoader/WICTextureLoader12.h" "WICTextureLoader/WICTextureLoader12.cpp" ${dxtex_files})

target_include_directories(Ext_DirectXTex PUBLIC "DDSTextureLoader" "WICTextureLoader" "DirectXTex")