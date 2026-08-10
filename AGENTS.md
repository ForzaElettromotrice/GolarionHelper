# Project Instructions

## Development Workflow

- Work in small, reviewable steps.
- Keep each change narrowly scoped to the piece explicitly requested by the user.
- Avoid broad refactors, unrelated cleanup, or multiple features in the same change.
- Ask before expanding the scope beyond the requested step.
- Work directly on the current branch unless the user explicitly requests another branch.
- Maintain `PROJECT_CONTEXT.md` when meaningful changes happen.

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

## Testing

- Keep the core independently compilable and testable without a frontend.
- Add focused tests alongside each new model component.

## Pathfinder Rules References

- Use the Golarion wiki as the primary Pathfinder rules and setting reference: <https://golarion.altervista.org/wiki/Pagina_principale>.
- Follow the English wiki reference linked from the relevant Golarion page for official English terminology.
