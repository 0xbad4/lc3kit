# Build

## Quick start

```bash
cmake -S . -B build
cmake --build build
```

This generates the CMake build tree in `build/` and compiles the project using C++20.

## CMake build

The project is configured in `CMakeLists.txt` as a header-only library target named `lc3kit`.

```bash
cmake -S . -B build
cmake --build build
cmake --install build
```

- Build directory: `build/`
- Install step: optional, installs headers and CMake package files
- Target: `lc3kit` (header-only interface library)

## Legacy Makefile

The repo also includes a simple Make-based workflow under `src/`.

```bash
cd src
make init
make asm
make vm
make test
make clean
```

### Produced binaries

`src/bin/` contains the generated executables:
- `lc3kit-asm`
- `lc3kit-vm`
- `test`

### Makefile details

- Compiler: `g++`
- Standard: `-std=c++20`
- Include path: `../include`
- Output directory: `src/bin`

## Notes

- The library is header-only, so no compiled library artifact is produced by CMake beyond the install metadata.
- The assembler and VM sample apps are built as standalone binaries for local testing and demos.
- For most users, the CMake path is the preferred build flow.