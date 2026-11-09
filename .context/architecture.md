# Architecture

- The domain model is implemented in modern C++.
- Rule categories are a closed system and must use enum classes. JSON may use stable English names, but parsers reject names outside the enum vocabulary. IDs and descriptive text remain strings. The current change aligns spell metadata and feat types/slot filters/Views; it does not silently migrate unrelated existing subsystems.
- `core/` is the backend module container; `core/CMakeLists.txt` adds `sheet` and `play`. The sheet engine, catalogs, persistence, C bindings, and tests live in `core/sheet/`; `core/play/` owns the independent C++20 session model, function registry, TCP presence prototype, and diagnostic executables.
- `golarion::play::Session` is the aggregate for play subsystems, analogous to `CharacterSheet`. Its constructor requires a host user ID. It owns ResourceManager first and RoleManager second, passing the registry by reference so it outlives the registered provider. Server ownership and the connected-user subsystem remain future steps; the existing TCP prototype remains separate.
- The application target is native desktop software in C++. A minimal API with C linkage already exposes the sheet through FFI. Whether the future desktop frontend uses that boundary or a typed C++ application interface remains open; see `game-management.md` for the game-management design proposal.
- C++ types, STL containers, ownership, and exceptions do not cross the C API boundary.
- Complex frontend snapshots may later be exchanged as JSON, but no serialization dependency is introduced yet.
- GUI snapshots use `View`/`toView`; future persistence structures will use `SaveData`/`toSaveData` to keep calculated presentation state separate from serialized source state.
- Character-sheet components register enhanceable resources by name; they neither create nor retain their `ModifierSet`.
- `golarion::play::ResourceManager` is shared only by reference among one Session's subsystems. It is separate from the sheet-specific `golarion::ResourceManager`; no sheet behavior is moved into it. Its implementation and tests live in `core/play/`.
- RoleManager implements user registration, role lookup, minimum-role checks, and authorized changes keyed by stable user IDs, exposing `roles.registerUser`, `roles.getRole`, `roles.hasAtLeastRole`, and `roles.setRole` through the session registry; see `session-roles.md` for exact signatures and lifecycle. The future ProtocolManager will expose `add_command(name, minimumRole, callback)` through that registry and check the trusted sender's current role before dispatch. Target-specific restrictions are also enforced by RoleManager's mutation operation.
- Automatic condition generation from independent character-sheet state is deliberately limited to `Barcollante`, `Privo di Sensi`, `Inabile`, `Morente`, `Stabilizzato`, and `Morto`. A condition definition may still generate another condition when the latter is a mandatory rules consequence, such as `Privo di Sensi` producing `Indifeso` or `Immobilizzato` producing `Impreparato`; the derived entry exists only while its source condition requires it. Every other character condition requires an explicit external source entry. `Impreparato` is not generated from encounter state because encounter-state integration is not implemented; a future play module may supply it, while `Rotto` is excluded because it is a condition of individual objects rather than of the character.

## Session ResourceManager

- `registerFeature<Signature>(name, callback)` owns a nonempty `std::function<Signature>` under a unique name. Callbacks must be copy constructible, as required by `std::function`.
- `invoke<Signature>(name, arguments...)` checks the exact registered signature, including return type and reference qualifiers, before forwarding arguments and returning the callback's result. Void and reference returns and move-only arguments are supported. Mutable callback state survives successive calls.
- `unregisterFeature(name)` removes the registration; the name can subsequently be reused with another signature. Calls already in progress retain the original callable until they return, including when a callback removes or replaces its own registration. Nested invocations are supported.
- Names are case sensitive, with outer ASCII whitespace trimmed consistently in all operations. Empty/whitespace-only names, empty callbacks, duplicate registrations, missing features, and signature mismatches throw `std::invalid_argument`. Callback exceptions propagate unchanged.
- A provider must unregister its callbacks before destroying the objects they reference. ResourceManager and Session cannot be copied or moved, preserving the address used by subsystem references. Access is serialized on the Session's execution thread; the registry does not add synchronization or interpret roles, TCP, or JSON.

```cpp
resources.registerFeature<int(int)>("example.double", [](int value) { return value * 2; });
const int result = resources.invoke<int(int)>("example.double", 3);
resources.unregisterFeature("example.double");
```
