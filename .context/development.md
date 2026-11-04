# Development

## Build and Test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The root build includes `core/sheet/` through `core/CMakeLists.txt`. Sheet test executables and libraries are generated under `build/core/sheet/`; target names and root build/test commands are unchanged.

## Dependencies

The JSON module requires the `nlohmann-json` package (`yay -S nlohmann-json` on Arch Linux).
