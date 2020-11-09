# VolumetricRaytracer
A raytracing renderer built upon DirectX12. It will support volumetric realtime raytracing.

**Please note!**
This application is in a very early stage of development.

## Installation

### Built from source
1. Ensure git 1.6.5 or later is installed on your machine
1. Download and install boost library. Version 1.74.0 was used in this project.
1. Build boost as a static library. *b2 variant=release link=static runtime-link=static stage*

## External dependencies
- [spdlog](https://github.com/gabime/spdlog) | *version* *1.8.1* | [MIT License](https://github.com/gabime/spdlog/blob/v1.x/LICENSE)
- [boost](https://www.boost.org) | *version* *1.74.0* | [Boost Software License](https://www.boost.org/users/license.html)
- [d3dx12](https://github.com/microsoft/DirectX-Graphics-Samples) | *version* *v1.5-dxr* | [MIT License](https://github.com/microsoft/DirectX-Graphics-Samples/blob/v1.5-dxr/LICENSE)
