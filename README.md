# LC3-kit (v1.0.1)

A LC-3 virtual machine and assembler.

## Components

- **VM** — LC-3 virtual machine with hook-based debugger API and the `LC3kit-ext` instruction set extension.
- **Assembler** — two-pass LC-3 assembler producing standard `.obj` files.

## Installation

### Prerequisites

- CMake 3.28 or higher
- C compiler (GCC/Clang on Linux, MSVC on Windows)

### Build and Install

**Linux:**

```bash
mkdir build && cd build
cmake ..
cmake --build .
cmake --install .
```

**Windows:**

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
cmake --install . --config Release
```

The library will be installed to the system CMake package directory and can be used in other projects via `find_package(lc3kit)`.

## Documentation

- [LC3kit-ext Encoding](docs/EXTENSION.md) - LC3kit-ext instruction encoding reference.
- [HTML Documentation](docs/web/index.html) - Local HTML documentation.
- [Live Documentation](https://0xbad4.github.io/lc3kit/) - HTML documentation hosted on GitHub Pages.
