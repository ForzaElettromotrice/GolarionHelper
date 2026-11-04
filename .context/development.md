# Development

## Build and Test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The root build includes `core/sheet/` and `core/play/` through `core/CMakeLists.txt`. Sheet executables/libraries are generated under `build/core/sheet/`, and play targets under `build/core/play/`. Existing sheet target names and root build/test commands are unchanged.

## Standalone Play Backend

The play module has no sheet/JSON/frontend dependency and can be configured independently:

```sh
cmake -S core/play -B build/play-cmake
cmake --build build/play-cmake --config Debug
ctest --test-dir build/play-cmake -C Debug --output-on-failure
```

The network process test is included when Python is available. The host and client diagnostic tools are C++ executables. In three separate Windows terminals, run the following commands from the workspace root (append `Debug/` to the executable directory for a multi-configuration CMake generator):

```powershell
./build/play-cmake/golarion_play_host.exe 42500 127.0.0.1 "Partita di prova"
./build/play-cmake/golarion_play_client.exe 127.0.0.1 42500 Alice
./build/play-cmake/golarion_play_client.exe 127.0.0.1 42500 Bob
```

Both clients show the current participant list and refresh it on joins and disconnects. Close with Ctrl+C. A host is not automatically a participant; run a client on its PC too if that person wants to join. For a LAN test, bind the host to its LAN IPv4 address or `0.0.0.0`, then connect clients to that PC's LAN IPv4 address; firewall rules must already permit the chosen port. The tools do not change firewall/router settings. Authentication, encryption, roles, and saved membership are not implemented yet.

CMake is currently absent from the execution environment. The verified Windows builds use the installed GCC directly and produce executables under `build/play/`:

```powershell
g++ -std=c++20 -Wall -Wextra -Wpedantic -I core/play/include core/play/src/game.cpp core/play/src/protocol.cpp core/play/src/network.cpp core/play/tools/host.cpp -lws2_32 -o build/play/golarion_play_host.exe
g++ -std=c++20 -Wall -Wextra -Wpedantic -I core/play/include core/play/src/game.cpp core/play/src/protocol.cpp core/play/src/network.cpp core/play/tools/client.cpp -lws2_32 -o build/play/golarion_play_client.exe
g++ -std=c++20 -Wall -Wextra -Wpedantic -I core/play/include core/play/src/game.cpp core/play/tests/game_test.cpp -o build/play/play_game_test.exe
g++ -std=c++20 -Wall -Wextra -Wpedantic -I core/play/include -I core/play/src core/play/src/protocol.cpp core/play/tests/protocol_test.cpp -o build/play/play_protocol_test.exe
./build/play/play_game_test.exe
./build/play/play_protocol_test.exe
python core/play/tests/network_process_test.py build/play/golarion_play_host.exe build/play/golarion_play_client.exe
```

Create `build/play/` before running these manual commands. The loopback process test needs local socket permissions; the current sandbox denies connects with WSA error 10013, while execution with those permissions passes. The POSIX transport branch has not been tested in this Windows environment.

## Dependencies

The JSON module requires the `nlohmann-json` package (`yay -S nlohmann-json` on Arch Linux).
