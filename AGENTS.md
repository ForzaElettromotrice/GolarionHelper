# Project Instructions

## Development Workflow

- Use newer Qt APIs when helpful. If a feature requires a newer Qt release, raise the frontend minimum in CMake and update the frontend documentation in the same change.
- Work in small, reviewable steps.
- Keep the repository organized by component and purpose. Reuse existing directories, keep generated build and packaging artifacts out of version control, and avoid unrelated reorganizations.
- Keep each change narrowly scoped to the piece explicitly requested by the user.
- Avoid broad refactors, unrelated cleanup, or multiple features in the same change.
- Ask before expanding the scope beyond the requested step.
- Do not change the backend, including its APIs and data, unless the user explicitly asks for a backend change. If a frontend task requires one, explain what is missing and ask before editing the backend.
- Work directly on the current branch unless the user explicitly requests another branch.
- Use `.context/README.md` as the project-context entry point.
- After meaningful changes, update only the relevant `.context/` documents and keep the index's current state and next steps aligned.

## External dependencies and packaging

- Resolve a dependency only for targets that need it. Prefer a compatible installed version; if missing, use a pinned, checksum-verified download from upstream under the build directory. Respect `GOLARION_ALLOW_DOWNLOADS=OFF`: reuse a local copy or explain what is missing. Do not install packages globally during a build.
- When adding a runtime dependency, update distribution in the same change: bundle the required runtime files on Windows and declare the corresponding system packages on Arch. Keep build-only and installer tools out of the shipped app. Verify the staged or packaged app and document any platform not tested.

## Language

- Write source code in English, including names and comments.
- Write user-facing interface text in Italian.
- Write user-facing assistant messages in Italian using proper accented characters.

## C++ Conventions

- Use C++20.
- Use `#pragma once` in header files.
- Prefer RAII and value semantics; avoid owning raw pointers.
- Keep the domain model independent from the C interoperability layer.
- Expose only opaque handles and C-compatible types through the public C API.
- Never allow C++ exceptions to cross the C API boundary.
- Keep all parameters on the same line in function and constructor declarations.
- Add public getters only when an actual call site needs them.
- Treat the rules domain as a closed system: always use enum classes for categories and rule types, never open strings. Strings remain appropriate for instance/catalog IDs and descriptive text. JSON names must be converted to enums at the input boundary, rejecting unknown values.

## Testing

- Keep the core independently compilable and testable without a frontend.
- Add focused tests alongside each new model component.

## Pathfinder Rules References

- Use the Golarion wiki as the primary Pathfinder rules and setting reference: <https://golarion.altervista.org/wiki/Pagina_principale>.
- Follow the English wiki reference linked from the relevant Golarion page for official English terminology.

## Project context

- Use `.context/` as the authoritative context directory in this project.
- Keep `.context/README.md` short: project goal, current state, next steps,
  and a description of the directory structure. Link each topic and explain
  when to read it. Read this index first, then only the relevant documents.
- Split details by topic. Create files only when needed; when a topic grows,
  give it a subdirectory with its own short README. Do not read the whole tree
  recursively or repeat the same details in multiple files.
- Keep current facts and decisions in the topic documents. Put useful past
  events and superseded decisions in `history/`, clearly dated and separate
  from current instructions. Verify old operational facts before relying on them.
- After meaningful changes, update the affected documents and any changed
  status, next steps, or structure in the index. Replace obsolete current
  statements instead of accumulating contradictory updates.
