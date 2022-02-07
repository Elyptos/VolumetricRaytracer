# VolumetricRaytracer
A raytracing renderer built upon DirectX12. It renders objects by using realtime volume raytracing.

**Please note!**
This application will not be actively maintained anymore.

## Overview
The project contains to applications:

- Volumetric Raytracer - This is the renderer application
- Voxelizer - This is a tool for converting gltf models into voxel volumes

## Installation

### Prebuilt binaries

You can find prebuilt binaries in the releases section of Github. Please make sure you
have a DirectX 12 supported graphics card and operating system in your machine.

### Built from source
1. Ensure cmake is installed
1. Ensure git 1.6.5 or later is installed on your machine
1. Download and install boost library. Version 1.74.0 was used in this project.
1. Build boost as a static library. *b2 variant=release link=static runtime-link=static stage*
1. Set environment path to boost directory. Variable name = BOOST_ROOT
1. Build the cmake project (below instructions for Windows)
* Make sure Windows 10 SDK is installed
* Open a command prompt
* Navigate to the projects root directory which contains the CMakeLists.txt file
* Initialize cmake project by running: *cmake .*
* Build: *cmake --build .*
1. There may be compilation errors with some sample code of gltf. In this case open ext/gltf/src/gtlf/GTLFSDK.Samples/Serialize/Source/main.cpp and add #define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING at the start of the file
1. Do the same with ext/gltf/src/gtlf/GTLFSDK.Samples/Deserialize/Source/main.cpp
1. Build again

## External dependencies
- [spdlog](https://github.com/gabime/spdlog) | *version* *1.8.1* | [MIT License](https://github.com/gabime/spdlog/blob/v1.x/LICENSE)
- [boost](https://www.boost.org) | *version* *1.74.0* | [Boost Software License](https://www.boost.org/users/license.html)
- [d3dx12](https://github.com/microsoft/DirectX-Graphics-Samples) | *version* *v1.5-dxr* | [MIT License](https://github.com/microsoft/DirectX-Graphics-Samples/blob/v1.5-dxr/LICENSE)
- [eigen](http://eigen.tuxfamily.org/index.php?title=Main_Page) | *version* *3.3.8* | [MPL License](https://www.mozilla.org/en-US/MPL/2.0/)
- [DirectXTex](https://github.com/microsoft/DirectXTex) | *version* *November 11, 2020* | [MIT License](https://github.com/microsoft/DirectXTex/blob/master/LICENSE)
- [gltf](https://github.com/microsoft/glTF-SDK) | *version* *r1.9.5.0* | [MIT License](https://github.com/microsoft/glTF-SDK/blob/master/LICENSE)
- [rapidjson](https://github.com/Tencent/rapidjson) | *version* *1.1.0* | [MIT License](https://github.com/Tencent/rapidjson/blob/master/license.txt)

## External resources
- [Skybox Texture](https://reije081.home.xs4all.nl/skyboxes/) | Roel Reijerse | [Attribution-NonCommercial-ShareAlike ](https://creativecommons.org/licenses/by-nc-sa/3.0/)
