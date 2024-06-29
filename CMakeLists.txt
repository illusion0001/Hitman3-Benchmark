cmake_minimum_required(VERSION 3.15)

project(BenchmarkStarter CXX)

# Find latest version at https://github.com/OrfeasZ/ZHMModSDK/releases
# Set ZHMMODSDK_DIR variable to a local directory to use a local copy of the ZHMModSDK.
set(ZHMMODSDK_VER "v2.0.0")
include(cmake/setup-zhmmodsdk.cmake)

# Set C++ standard to C++23.
set(CMAKE_CXX_STANDARD 23)

# Create the BenchmarkStarter mod library.
add_library(BenchmarkStarter SHARED
    src/BenchmarkStarter.cpp
    src/BenchmarkStarter.h
)

# Add SDK and DX headers dependencies.
find_package(directx-headers CONFIG REQUIRED)

target_link_libraries(BenchmarkStarter
    ZHMModSDK
    Microsoft::DirectX-Guids
    Microsoft::DirectX-Headers
)

install(TARGETS BenchmarkStarter
    RUNTIME DESTINATION bin
)

# Install the mod to the game folder when the `GAME_INSTALL_PATH` variable is set.
zhmmodsdk_install(BenchmarkStarter)
