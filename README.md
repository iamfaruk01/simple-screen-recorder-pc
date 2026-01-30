# Simple Screen Recorder PC

A lightweight, high-performance screen recording application for Windows built with C++20 and modern graphics APIs.

## 🎯 Features

- **Screen Capture**: High-quality desktop recording using DirectX 11
- **Audio Recording**: Simultaneous audio capture from system audio and microphone
- **Webcam Support**: Picture-in-picture webcam overlay during recording
- **Visual Effects**: Real-time annotations, highlights, and cursor effects
- **Region Selection**: Select specific screen areas for focused recording
- **Video Encoding**: Efficient MP4 output with configurable quality settings
- **Modern UI**: Clean, responsive interface built with native Windows APIs

## 🏗️ Architecture

The application is built with a modular architecture:

```
src/
├── main.cpp              # Application entry point and main loop
├── ScreenCapture.cpp     # DirectX-based screen capture engine
├── VideoEncoder.cpp      # FFmpeg video encoding wrapper
├── AudioCapture.cpp      # Windows audio capture (WASAPI)
├── VisualEffects.cpp     # Real-time visual effects and annotations
├── Controller.cpp        # Main application controller and UI logic
├── RegionSelector.cpp    # Screen region selection interface
└── WebcamDevice.cpp      # Webcam capture and overlay management

include/
├── ScreenCapture.hpp
├── VideoEncoder.hpp
├── AudioCapture.hpp
├── VisualEffects.hpp
├── Controller.hpp
├── RegionSelector.hpp
└── WebcamDevice.hpp
```

## 🚀 Getting Started

### Prerequisites

- **Windows 10/11** (x64)
- **Visual Studio 2022** or newer with C++20 support
- **CMake 3.20** or newer
- **vcpkg** package manager
- **FFmpeg** libraries (via vcpkg)

### Dependencies

The project uses the following key dependencies:

```bash
# Install via vcpkg
vcpkg install ffmpeg:x64-windows
vcpkg install directx-headers:x64-windows
vcpkg install boost-asio:x64-windows  # For async operations (if needed)
```

### Building the Project

1. **Clone the repository**
   ```bash
   git clone https://github.com/your-username/simple-screen-recorder-pc.git
   cd simple-screen-recorder-pc
   ```

2. **Setup vcpkg**
   ```bash
   # If you don't have vcpkg installed
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.bat
   ./vcpkg integrate install
   ```

3. **Install dependencies**
   ```bash
   vcpkg install ffmpeg:x64-windows
   ```

4. **Configure and build**
   ```bash
   # Create build directory
   mkdir build
   cd build

   # Configure with CMake
   cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake

   # Build the project
   cmake --build . --config Release
   ```

5. **Run the application**
   ```bash
   # From the build directory
   ./Release/SimpleScreenRecorder.exe
   ```

## 🛠️ Development Setup

### IDE Configuration

**Visual Studio 2022:**
1. Open the CMakeLists.txt file as a CMake project
2. Set the toolchain file to your vcpkg installation
3. Select `x64-Release` configuration
4. Build and run

**VS Code:**
1. Install the C/C++ and CMake Tools extensions
2. Configure `settings.json` with vcpkg paths
3. Use the integrated terminal for build commands

### Code Style

This project follows modern C++20 conventions:
- Use `std::` namespace explicitly
- Prefer RAII and smart pointers
- Follow camelCase for variables, PascalCase for classes
- Include guards use `#pragma once`
- Keep functions focused and under 50 lines when possible

### Adding New Features

1. **Create a new class** in `include/` and `src/`
2. **Update CMakeLists.txt** to include new source files
3. **Add unit tests** (when test framework is added)
4. **Update documentation** in this README

## 🤝 Contributing

We welcome contributions! Here's how to get started:

### Reporting Issues

- Use GitHub Issues for bug reports and feature requests
- Include system information (Windows version, hardware specs)
- Provide detailed steps to reproduce issues
- Include screenshots or screen recordings for UI issues

### Pull Request Process

1. **Fork the repository** and create a feature branch
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes** following the code style guidelines
3. **Test thoroughly** on different Windows versions
4. **Update documentation** if needed
5. **Commit with descriptive messages**
   ```bash
   git commit -m "feat: Add webcam overlay support"
   ```

6. **Push and create a pull request**
   ```bash
   git push origin feature/your-feature-name
   ```

### Development Guidelines

- **Code Quality**: Ensure all code compiles without warnings on MSVC
- **Performance**: Test recording performance on different hardware configurations
- **Memory Safety**: Use RAII, avoid raw pointers, check for memory leaks
- **Thread Safety**: Use proper synchronization for multi-threaded components
- **Error Handling**: Provide meaningful error messages and graceful fallbacks

### Testing

Currently, manual testing is required. We plan to add:
- Unit tests for core components
- Integration tests for recording workflows
- Performance benchmarks
- Automated UI testing

## 📁 Project Structure

```
simple-screen-recorder-pc/
├── CMakeLists.txt          # Main build configuration
├── README.md              # This file
├── .gitignore             # Git ignore patterns
├── include/               # Header files
├── src/                   # Source implementation
├── build/                 # Build output directory
└── docs/                  # Documentation (planned)
```

## 🔧 Configuration

The application supports runtime configuration through:

- **Command line arguments** (planned)
- **Configuration file** (JSON format, planned)
- **UI settings** (in-app preferences)

### Default Settings

- Output format: MP4 (H.264 + AAC)
- Frame rate: 30 FPS
- Resolution: Native display resolution
- Audio: 48kHz, stereo, 128 kbps

## 🐛 Troubleshooting

### Common Issues

**Build fails with FFmpeg errors:**
```bash
# Ensure vcpkg is properly integrated
vcpkg integrate install
# Clean and rebuild
rm -rf build/
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-path]/scripts/buildsystems/vcpkg.cmake
```

**Screen capture not working:**
- Ensure graphics drivers are up to date
- Run as administrator if needed
- Check DirectX 11 support

**Audio recording issues:**
- Verify microphone permissions in Windows settings
- Check audio device availability in Sound Control Panel

### Debug Mode

Build with debug symbols for troubleshooting:
```bash
cmake --build . --config Debug
```

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **FFmpeg** - For video/audio encoding capabilities
- **DirectX 11** - For high-performance screen capture
- **WASAPI** - For Windows audio capture
- **vcpkg** - For C++ package management

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/your-username/simple-screen-recorder-pc/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-username/simple-screen-recorder-pc/discussions)
- **Email**: your-email@example.com

---

**Happy Recording! 🎥**
