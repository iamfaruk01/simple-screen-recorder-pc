cmake_minimum_required(VERSION 3.20)
project(SimpleScreenRecorder VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/ScreenCapture.cpp
    src/VideoEncoder.cpp
    src/VisualEffects.cpp
    src/AudioCapture.cpp
    src/Controller.cpp
    src/RegionSelector.cpp
    src/WebcamDevice.cpp
    src/resources.rc
)

set(HEADERS
    include/ScreenCapture.hpp
    include/VideoEncoder.hpp
    include/VisualEffects.hpp
    include/AudioCapture.hpp
    include/Controller.hpp
)

# On Windows, we need these libraries for screen capture
if(WIN32)
    set(WIN_LIBS
        d3d11
        dxgi
        dwmapi
    )
endif()

# FFmpeg libraries (Assuming they will be installed via vcpkg or similar)
# For now, we'll just define the structure
# find_package(FFMPEG REQUIRED)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE include)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${WIN_LIBS})
endif()
