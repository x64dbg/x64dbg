## Prerequisites
```sh
sudo apt update
sudo apt install cmake ninja-build wine
```

## Install MSVC
Download https://github.com/mstorsjo/msvc-wine
```sh
cd msvc-wine
./vsdownload.py --accept-license --dest ~/opt/msvc Microsoft.VisualStudio.Workload.VCTools Microsoft.VisualStudio.Component.VC.ATL
./install.sh ~/opt/msvc
```

## Build
x86
```sh
cd x64dbg
export MSVC_BIN_DIR=~/opt/msvc/bin/x86
export QT_BIN_DIR=~/src/x64dbg/build32/_deps/qt5-src/bin
cmake -B build32 -DCMAKE_TOOLCHAIN_FILE=cmake/msvc-wine.cmake -G Ninja
cmake --build build32 -j4
```

x64
```sh
cd x64dbg
export MSVC_BIN_DIR=~/opt/msvc/bin/x64
export QT_BIN_DIR=~/src/x64dbg/build64/_deps/qt5-src/bin
cmake -B build64 -DCMAKE_TOOLCHAIN_FILE=cmake/msvc-wine.cmake -G Ninja
cmake --build build64 -j4
```

## Issues
- ```LINK : fatal error LNK1158: cannot run 'rc.exe'```
> Fix: winecfg -> Drives -> Remove drives with alternative path to x64dbg src (like: E: -> ~/src)

