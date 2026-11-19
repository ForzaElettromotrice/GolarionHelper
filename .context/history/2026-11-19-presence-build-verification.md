# Presence Build Verification — 2026-11-19

During connected-participant implementation, an incremental test executable failed with exit code 0x20474343 while rethrowing an expected ProtocolError through future<json>::get. The two existing command/future suites still passed. Debugger output located the failure at the participant test's invalid-payload rejection check; no participant-list invariant failure was reported. The precise cause was not established.

A clean build from the repository-root CMake configuration rebuilt the play library, tools and tests. The participant suite then passed, including three consecutive repeat runs, without a runtime code workaround for this failure. This is a historical observation about that build, not evidence of a general compiler defect.

Commands used from the repository root, with the configured compiler/dependencies and Windows runtime path:

    cmake --build build --clean-first --target golarion_play_host golarion_play_client play_participants_test --parallel 4
    ctest --test-dir build/core/play -R '^play_participants_test$' --repeat until-fail:3 --output-on-failure

The actual clean invocation also selected all remaining play test targets before the complete CTest run. Current build instructions and verification results belong in development.md and session-participants.md.
