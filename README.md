xp12-fmod-sdk-demo
=========================

[![CMake](https://github.com/sparker256/xp12-fmod-sdk-demo/actions/workflows/cmake.yml/badge.svg)](https://github.com/sparker256/xp12-fmod-sdk-demo/actions/workflows/cmake.yml)

## Building xp12-fmod-sdk-demo from source

### If you use the Fmod Library understand that it is NOT a free library.

### Remember to follow the rules of [FMOD's license](https://www.fmod.com/licensing) when using this library


### On windows

1.  Install required software using [Chocolatey](https://chocolatey.org/) using admin command prompt:

    ```
    choco install git cmake
    choco install mingw --version 8.1.0
    ```

    You can also install the same programs manually if you prefer.

2.  Checkout and configure the project:

    ```
    git clone https://github.com/sparker256/xp12-fmod-sdk-demo.git
    cd xp12-fmod-sdk-demo
    cmake -G "MinGW Makefiles" -S .\src -B .\build -DCMAKE_BUILD_TYPE=RelWithDebInfo
    ```

3.  Build the project and copy the plugin DLL into the appropriate directory:

    ```
    cmake --build .\build
    mkdir .\xp12-fmod-sdk-demo\win_x64
    copy .\build\win.xpl .\xp12-fmod-sdk-demo\win_x64\xp12-fmod-sdk-demo.xpl
    ```

### On Ubuntu:

1. Install required software:

   ```
   sudo apt-get install -y --no-install-recommends build-essential cmake git freeglut3-dev libudev-dev libopenal-dev

   ```

2. Checkout and configure the project:

   ```
   git clone https://github.com/sparker256/xp12-fmod-sdk-demo.git
   cd xp12-fmod-sdk-demo
   cmake -S ./src -B ./build -DCMAKE_BUILD_TYPE=RelWithDebInfo
   ```

3. Build the project and copy the plugin DLL into the appropriate directory:

   ```
   cmake --build ./build
   mkdir ./xp12-fmod-sdk-demo/lin_x64
   cp ./build/lin.xpl ./xp12-fmod-sdk-demo/lin_x64/xp12-fmod-sdk-demo.xpl
   ```

### On OS X:

1. Install XCode, Git, CMake (Homebrew can be convenient for this).

2. Checkout and configure the project:

   ```
   git clone https://github.com/sparker256/xp12-fmod-sdk-demo.git
   cd xp12-fmod-sdk-demo
   cmake -S ./src -B ./build -DCMAKE_BUILD_TYPE=RelWithDebInfo
   ```

3. Build the project and copy the plugin DLL into the appropriate directory:

   ```
   cmake --build ./build
   mkdir ./xp12-fmod-sdk-demo/mac_x64
   cp ./build/mac.xpl ./xp12-fmod-sdk-demo/mac_x64/xp12-fmod-sdk-demo.xpl
   ```
