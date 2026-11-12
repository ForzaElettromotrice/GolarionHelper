# Admission Routing Before Unified Dispatch

Archived 2026-11-12. These observations describe the implementation before join/resume became commands with minimum Any. Current behavior is documented in [../session-commands.md](../session-commands.md) and [../session-flows.md](../session-flows.md).

The communication review reproduced a namespace collision: `add_command("session.join", ...)` succeeded, but the wire router handled the exact name as admission before dispatch. After admission, the exact name returned `ALREADY_ADMITTED`, while `" session.join "` reached the registered callback because only dispatch trimmed whitespace. Join/resume also returned separate `session.joined` / `session.resumed` envelopes with direct payload fields.

The initial proposal was to reserve control names separately. The user instead chose a common registry with `Role::Any`, explicitly nonassignable, and optional identity for live pending connections. ProtocolManager now registers both built-ins first via `add_command`, uses common normalization/duplicate checks/role lookup, and wraps their callback results in `request.accepted` / `payload.result`.

The old reproduction is in ignored `build/play/communication-review/review_probe.cpp`, linked against the older `command-dispatch` build. Its reserved-name probe assumes registration succeeds and is not a valid acceptance test for current code. Current ProtocolManager/admission tests cover duplicate names, padded variants, anonymous contexts, and common responses. The separate buffered-stop finding was still open at the end of this step; its subsequent correction is recorded in [2026-11-12-buffered-stop.md](2026-11-12-buffered-stop.md).
