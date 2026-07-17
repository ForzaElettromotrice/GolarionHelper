# GolarionHelper Project Context

## Goal

Build a JavaFX desktop helper for Pathfinder 1e character sheets. The application is being developed domain-first: the character model handles derived statistics, modifiers, conditional effects, reversible actions, attacks, equipment, carrying load, and spell/item catalogs, while the UI exposes these capabilities incrementally.

## Current State

- The main UI currently exposes character abilities, saving throws, hit points, initiative, armor class, skills, and enhancement groups.
- Attack, equipment, item, and spell domain models exist but are not fully exposed in the UI.
- Runtime spell and item JSONL resources are currently empty; the legacy spell dataset remains under `scripts/`.
- There are currently no automated tests.
- The build requires Java 21 and uses the Gradle wrapper, JavaFX 21, Gson, Lombok, and the Beryx jlink plugin.
- Arch Linux is supported by generating a self-contained application image without a native installer. Windows continues to generate an MSI installer.

## Important Decisions

- Pathfinder 1e rules use the Golarion wiki as the primary reference, as documented in `docs/conventions.md`.
- Domain mutations go through `CharacterSheet`; views consume DTO records from `model/api`.
- Effects use named targets and expressions, and equipment/size/load behavior is represented with reversible actions.
- Packaging must remain platform-specific. Windows produces an MSI installer; Arch Linux currently produces a self-contained application image. A native Arch Linux package would require a separate PKGBUILD/AUR workflow.

## Development Setup

1. Install OpenJDK 21. On Arch Linux: `yay -S jdk21-openjdk`.
2. Use the project wrapper rather than a globally installed Gradle: `./gradlew`.
3. If Java 21 is not the system default, run Gradle with `JAVA_HOME=/usr/lib/jvm/java-21-openjdk`.
4. Compile and verify with `./gradlew test`.
5. Run the JavaFX application with `./gradlew run`.
6. Create the self-contained Arch Linux application image with `./gradlew jpackageImage`.

## Environment Verified on 2026-07-17

- Arch Linux x86_64.
- OpenJDK 21.0.11 installed alongside the existing default OpenJDK 26.
- JDK 21 includes `javac`, `jlink`, and `jpackage`.
- GTK 3, X11 runtime libraries, ALSA, and `fakeroot` are installed.
- Gradle wrapper 9.0.0 and its plugin dependencies download successfully.
- `gradlew` is now executable.
- The Beryx `jpackage` configuration recognizes Linux and skips native installer generation, allowing `jpackageImage` to produce an Arch-compatible application directory.
- `./gradlew test jpackageImage` completes successfully.
- The generated image is available at `app/build/jpackage/Golarion` and is approximately 64 MB with its Java runtime included.
- The packaged launcher `app/build/jpackage/Golarion/bin/Golarion` starts successfully on the current Arch Linux graphical session.

## Pending Work

- Decide whether to add a native Arch PKGBUILD in the future.
- Verify `./gradlew run` in a graphical session when needed.
- Add automated tests, persistence, and real JSONL catalog data.
