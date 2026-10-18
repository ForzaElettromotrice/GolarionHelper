# Development

## Build and Test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Dependencies

The JSON module requires the `nlohmann-json` package (`yay -S nlohmann-json` on Arch Linux).
