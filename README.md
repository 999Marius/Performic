# Performic

An Android application built with Kotlin and C++ (via NDK) that combines native performance with modern Android development.

## 📋 Overview

Performic is an Android game application that leverages both Kotlin and native C++ code to deliver high performance. The app features data visualization capabilities using charts and follows modern Android development practices.

## ✨ Features

- **Native C++ Integration**: Uses Android NDK for performance-critical operations
- **Modern Android Architecture**: Built with Kotlin and AndroidX libraries
- **Data Visualization**: Integrated chart library for displaying performance metrics
- **Material Design**: Modern UI following Material Design guidelines
- **View Binding**: Type-safe view access for cleaner code

## 🛠️ Tech Stack

### Languages
- **Kotlin**: Primary application language
- **C++**: Native code for performance-critical components
- **Java**: Version 11 compatibility

### Key Technologies
- **Android NDK**: Version 29.0.14206865
- **CMake**: Version 3.22.1 for native build configuration
- **Gradle**: Kotlin DSL build system

### Libraries & Dependencies

#### UI & Core
- AndroidX Core KTX
- AndroidX AppCompat
- Material Components
- ConstraintLayout
- View Binding

#### Data & Visualization
- Gson 2.10.1 - JSON serialization/deserialization
- MPAndroidChart v3.1.0 - Chart and graph visualization
- TensorFlow Lite Acceleration Service

#### Testing
- JUnit
- AndroidX JUnit
- Espresso Core

## 📱 Requirements

- **Minimum SDK**: 24 (Android 7.0)
- **Target SDK**: 36
- **Compile SDK**: 36
- **Java**: Version 11

## 🚀 Getting Started

### Prerequisites

1. Android Studio (latest version recommended)
2. Android NDK version 29.0.14206865
3. CMake version 3.22.1 or higher
4. JDK 11

### Building the Project

1. Clone the repository:
```bash
git clone https://github.com/999Marius/Performic.git
cd Performic
```

2. Open the project in Android Studio

3. Sync Gradle files to download dependencies

4. Build the project:
```bash
./gradlew build
```

5. Run on device or emulator:
```bash
./gradlew installDebug
```

## 📁 Project Structure

```
Performic/
├── app/
│   ├── src/
│   │   ├── main/
│   │   │   ├── cpp/           # Native C++ code
│   │   │   ├── java/          # Kotlin/Java source files
│   │   │   ├── res/           # Resources (layouts, drawables, etc.)
│   │   │   └── AndroidManifest.xml
│   │   └── ...
│   ├── build.gradle.kts       # App module build configuration
│   └── proguard-rules.pro     # ProGuard rules
├── gradle/                    # Gradle wrapper files
├── build.gradle.kts           # Project build configuration
├── settings.gradle.kts        # Project settings
└── gradlew                    # Gradle wrapper script
```

## 🔧 Configuration

### Native Build

Native C++ code is configured in `CMakeLists.txt` located at:
```
app/src/main/cpp/CMakeLists.txt
```

### ProGuard

ProGuard rules for release builds are defined in `app/proguard-rules.pro`

## 📊 App Details

- **Package**: `com.example.performic`
- **Version**: 1.0 (Version Code: 1)
- **Category**: Game
- **Orientation**: Portrait

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📄 License

This project does not currently have a specified license.

## 👤 Author

**999Marius**

- GitHub: [@999Marius](https://github.com/999Marius)

## 📝 Notes

- The app uses portrait orientation only
- Native code integration via CMake
- View binding is enabled for type-safe view access
- ProGuard is currently disabled in release builds

---

Created on: November 11, 2025  
Last Updated: January 14, 2026
