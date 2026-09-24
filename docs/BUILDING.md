# Building OFXR Bridge

OFXR Bridge currently targets 64-bit Windows and builds with Visual Studio
2022, CMake 3.24 or newer and a recent Windows SDK containing `fxc.exe`.

## Dependencies

The repository already contains the exact OpenXR headers and NVIDIA Optical
Flow interface headers used by the project. NVIDIA's runtime API is supplied by
the installed display driver and is not required at build time.

FidelityFX SDK v1.1.4 is intentionally not committed. Clone the official SDK
at the pinned revision and build its static DX12 Optical Flow components:

```powershell
git clone --branch v1.1.4 --depth 1 `
  https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK.git `
  external/FidelityFX-SDK-v1.1.4

cmake -S external/FidelityFX-SDK-v1.1.4/sdk `
  -B build-fidelityfx -G "Visual Studio 17 2022" -A x64 `
  -DFFX_ALL=OFF -DFFX_OF=ON -DFFX_API_BACKEND=DX12_X64 `
  -DFFX_BUILD_AS_DLL=OFF

cmake --build build-fidelityfx --config Release
```

The expected outputs are:

```text
external/FidelityFX-SDK-v1.1.4/sdk/bin/ffx_sdk/ffx_backend_dx12_x64.lib
external/FidelityFX-SDK-v1.1.4/sdk/bin/ffx_sdk/ffx_opticalflow_x64.lib
```

The pinned FidelityFX commit is
`c6efa6bf7f2027b3ec94f28578bb5965eabb9e55`.

## Build and test

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The distributable files are generated under `build/Release`:

```text
OFXRBridgeTray.exe
ofxr/
  XR_APILAYER_XRFrameBridge_diagnostic.dll
  ofxr_bridge.ini
```

Do not distribute PDBs, static libraries, test executables or the NVIDIA SDK.

## Building from WSL

Use the Windows CMake executable and Visual Studio toolchain from WSL Bash.
This produces Windows binaries; Linux CMake/GCC is not the Windows release
toolchain. Install the Windows prerequisites above and prepare FidelityFX using
the same Windows tools. Pass Windows paths to Windows executables.

For this checkout on drive F (adjust paths for your own checkout):

```bash
'/mnt/c/Program Files/CMake/bin/cmake.exe' \
  -S F:/ai/GITHUB/OFXR-Bridge -B F:/ai/GITHUB/OFXR-Bridge/build \
  -G 'Visual Studio 17 2022' -A x64
'/mnt/c/Program Files/CMake/bin/cmake.exe' \
  --build F:/ai/GITHUB/OFXR-Bridge/build --config Release --parallel 6
'/mnt/c/Program Files/CMake/bin/ctest.exe' \
  --test-dir F:/ai/GITHUB/OFXR-Bridge/build -C Release --output-on-failure
```

For NVIDIA hardware verification, run from Windows PowerShell after building:

```powershell
$env:XRFG_TEST_NVIDIA = '1'
& .\build\Release\xrfg_d3d12_history_tests.exe
Remove-Item Env:XRFG_TEST_NVIDIA
```

## Packaging the fork

After building/testing a clean committed checkout, run Python 3 (WSL or Windows):

```bash
python3 scripts/package-release.py --version 0.2.1-perf.1
```

The script packages only the tray, layer, default configuration, instructions,
diagnostic scripts and licenses into `build/packages`. It records the source
commit and hashes, excludes debug/test binaries and raw logs, and checks that
the tray's layer copy matches the main build DLL. Build/test before packaging;
packaging alone does not prove source/binary correspondence.
