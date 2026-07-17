# LPG-cpp-runtime

C++17 runtime (`cpplpg2`) for [LPG2](https://github.com/A-LPG/LPG2).

## Install / coordinates

| Field | Value |
|-------|-------|
| Package | CMake library `cpplpg2` (source) |
| Version | 1.0.0 |
| Compatible generator | LPG2 ≥ 2.3.0 — see [`ecosystem/compat.json`](https://github.com/A-LPG/LPG2/blob/main/ecosystem/compat.json) |

```cmake
add_subdirectory(path/to/LPG-cpp-runtime)
target_link_libraries(your_parser PRIVATE cpplpg2)
```

## Minimum toolchain

C++17, CMake ≥ 3.8, ICU (for some stream APIs).

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Wiring generated files

1. Generate with `-programming_language=rt_cpp` (or `cpp` / `c++`) and `dtParserTemplateF.gi`
2. Compile generated `*.cpp` / headers with this library
3. Sample: https://github.com/A-LPG/LPG2/tree/main/examples/calculator/cpp

## Features

| Feature | Status |
|---------|--------|
| Deterministic parser | yes |
| Backtracking | yes |
| Nested automatic AST | yes |
| `%Recover` prosthetic AST | yes |

## Publish status

- Channel: source / CMake (Conan/vcpkg planned)
- Automation: CI build only

## Links

- Generator: https://github.com/A-LPG/LPG2
- Ecosystem: https://github.com/A-LPG/LPG2/blob/main/docs/ECOSYSTEM.md
