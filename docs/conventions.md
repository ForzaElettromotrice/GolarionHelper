# Project Conventions

## Rules References

- For Pathfinder 1e rules and setting information, use the Golarion wiki as the primary source:
  - https://golarion.altervista.org/wiki/Pagina_principale
- For English reference names, follow the link at the bottom of the relevant Golarion page to the corresponding English source page.

## Assistant Communication

- User-facing assistant messages must be written in Italian.
- Use proper accented Italian characters in assistant messages. Do not replace accented letters with ASCII approximations.
- Source code must be written in English, including class names, method names, variable names, package names, and comments in code unless there is a project-specific reason to do otherwise.
- Keep technical identifiers, code symbols, class names, method names, package names, and source code in their original form.
- Be concise and pragmatic in explanations unless the user explicitly asks for more detail.
- When the user asks about an approach or design, discuss and create only the explicitly requested piece first. Do not immediately implement or wire it into the rest of the system unless the user asks to proceed.

## Java Conventions

- Use Lombok annotations where appropriate to reduce boilerplate.
- Prefer `@Getter` and `@Setter` instead of manually writing trivial accessors when that matches the existing code style.
- For enums, always add `@Getter` when they expose fields such as `displayName` or similar metadata, since those accessors are expected to be used.
- For regular classes, do not add getters preemptively. Add them only when there is an actual call site that needs them.
- Prefer `@NonNull` on parameters and fields where null values are not allowed, especially in public APIs and domain model methods.
