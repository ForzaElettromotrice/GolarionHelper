# Architecture

- The domain model is implemented in modern C++.
- A thin API with C linkage will be the only boundary exposed to a future frontend through FFI.
- C++ types, STL containers, ownership, and exceptions do not cross that boundary.
- Complex frontend snapshots may later be exchanged as JSON, but no serialization dependency is introduced yet.
- GUI snapshots use `View`/`toView`; future persistence structures will use `SaveData`/`toSaveData` to keep calculated presentation state separate from serialized source state.
- Character-sheet components register enhanceable resources by name; they neither create nor retain their `ModifierSet`.
- Automatic condition generation from independent character-sheet state is deliberately limited to `Barcollante`, `Privo di Sensi`, `Inabile`, `Morente`, `Stabilizzato`, and `Morto`. A condition definition may still generate another condition when the latter is a mandatory rules consequence, such as `Privo di Sensi` producing `Indifeso` or `Immobilizzato` producing `Impreparato`; the derived entry exists only while its source condition requires it. Every other character condition requires an explicit external source entry. `Impreparato` is not generated from encounter state because no combat-state tracker is planned, while `Rotto` is excluded because it is a condition of individual objects rather than of the character.
