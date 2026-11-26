# ServerProtocol and ClientProtocol

ServerProtocol and ClientProtocol are independent implementations, not modes of a common manager. They share wire types, framing and small stateless utilities. Session owns PermissionManager only on hosts; JSON v4 and archive v4 use numeric masks. See session-participants.md, session-events.md and session-permission-commands.md.

## API and ResourceManager

- ServerProtocol registers join/resume with Permission::NoAuth, then publishes protocol.add_command, protocol.sendEvent and internal identity.updateUserPermissions. Host Session asks PermissionManager to register the permission adapters. ClientProtocol registers permissions.changed and publishes protocol.on_event with OnEventSignature (no requirements argument). Each Session owns a separate registry; registry features and network commands remain distinct.
- AddCommandSignature is void(std::string_view, CommandRequirements, CommandCallback). add_command validates a unique case-sensitive name with outer ASCII whitespace trimmed, requirements and a nonempty callback. Duplicate built-in names, including padded variants, cannot override admission.
- CommandRequirements is in golarion/play/command_requirements.hpp. A single Permission or a combined PermissionSet converts to requirements. Permission::NoAuth explicitly selects Public access with an empty mask. Permission::Any, a default requirement or PermissionSet{} selects Authenticated access, also with an empty mask. Public access with nonempty flags and unknown CommandAccess enum values reject at registration.
- dispatch(name, context, payload) remains a direct synchronous method, not a registry feature. It requires an object payload and a recognized local identity or live connection. For an identified user it invokes permissions.hasPermissions with the required set, requiring all flags at execution time. Unknown users/provider failures yield CommandFailed; denied flags or anonymous authenticated commands yield Forbidden. A live anonymous peer may execute an explicitly public command without participant permission lookup.
- Callbacks receive the trusted context and payload and return any JSON result. Their mutable state persists. Nested dispatch/registration remain supported. Target-specific mutations are checked by PermissionManager or the relevant subsystem, including host-only ChangePermissions management and protection of self/host targets.

ClientProtocol::on_event registers void handlers in its own event map; dispatchEvent routes incoming notifications without using server commands. Client dispatch validates its admitted connection generation without inventing a participant sender or invoking a local PermissionManager. Results are discarded for notifications. SessionClient exposes on_event; SessionServer exposes add_command and sendEvent. See session-events.md for the mode-specific signatures and routing.

Host registerParticipantCommands adds session.getParticipants with authenticated zero-bit access and an exact empty payload. Private getParticipants derives a revisioned projection from current bindings plus the local host. publishParticipantsChanged/sendParticipantsSnapshot reuse sendEvent; admitNewUser, bindUser and closeConnection call them after successful state changes and outside connection/identity locks. Client registerClientCommands adds session.participantsChanged, with a strict parser and ClientConnection::updateParticipants. No second participant registry or additional ResourceManager service is introduced; see session-participants.md.

```cpp
resources.invoke<ServerProtocol::AddCommandSignature>("protocol.add_command", "example.echo", Permission::Permission1 | Permission::Permission3, [](const CommandContext &context, const nlohmann::json &payload)
{
    return nlohmann::json{{"sender", *context.userId}, {"value", payload.at("value")}};
});
```

## Context, Execution and Lifecycle

CommandContext contains optional UserId and ConnectionId, never a permission cache or token. On hosts, a pending peer has only ConnectionId; makeContext checks record existence and both binding directions, and dispatch revalidates the actor. On clients, a notification context has only the current host-connection generation and is checked against the admitted ClientConnection. Payload identity/mask fields never define the sender. Host logical connection methods remain registerConnection, bindUser, getUserId, makeContext and closeConnection; Session delegates all except getUserId. Binding trusts an identity verified by its caller.

One recursive execution mutex serializes registration, local/remote dispatch, admission, binding and closing. Host and guest requests both run on Hosting; direct dispatch runs on its caller. No connection/permission lock is held across an arbitrary command callback. Providers called independently retain their own threading contracts.

SessionClient::sendCommand(name, payload) returns future<json> for host and guests over TCP. Same-manager callbacks cannot call sendCommand, preventing actor escalation and worker deadlock; nested dispatch with the original context remains available. See session-commands.md and session-client.md.

startHosting opens numeric IPv4/port and starts the private TCP worker; port zero returns an OS-assigned port. stopHosting requests stop, joins and closes sockets/bindings while preserving identities, permissions, credentials, code and the logical-ID counter. It does not disable local commands. The final remote checkpoint acquires executionMutex and rechecks stop before handleMessage: buffered requests or requests waiting behind local execution are discarded. An already executing callback finishes; delivery of its final reply is not guaranteed.

startHosting, stopHosting, loadIdentityState and client join/resume/disconnect reject calls from the same manager's callbacks before lifecycle locks. Stop joins without holding the execution lock. sendEvent uses a separate outbox mutex and never takes hostingMutex_; client snapshot/dispatch uses an execution-protected weak connection reference without clientMutex_. Admission lock order is execution, identity, connections, permissions; archive load adds lifecycle before execution. Save/issuance retain identity-then-permission ordering. Queue limits and notification ordering are documented in session-events.md.

## Ownership and Other Operations

SessionServer owns a host-mode protocol instance and nonoptional permissions and sheets. ClientProtocol owns its connection, mutex and request counter directly. The server destructor calls private ServerProtocol shutdown before PermissionManager destruction. Shutdown joins networking, cancels queued local requests and joins an active local callback. Direct ServerProtocol owners must keep providers alive through shutdown. Fixed registry registration/removal is not concurrent with workers or feature invocation. Managers are noncopyable/nonmovable.

SessionCode remains five uppercase alphanumeric symbols, with lowercase accepted. It is stable during a live Session's hosting restarts; host-wide uniqueness and code persistence remain deferred. Clients supply the remote code. Wrong-mode methods throw logic_error; noexcept verifySessionCode returns false on clients.

Token issuance, recognition, save/load and archive ownership remain in ServerProtocol. IdentitySaveData now includes hostId and UserPermissionsSaveData plus tokens. Trusted load checks the explicit hostId and immutable host mask before replacing state. No legacy importer exists. See session-tokens.md.

Hosting and ClientConnection remain private implementations. Socket/runtime helpers are shared; the JSON codec is unaware of permissions. Standard C++20, nlohmann_json 3.12, and Threads are used, plus ws2_32 and bcrypt on Windows. The module remains independent of sheet/frontend.

## Verification

The repository-root Debug/Ninja CMake build passes seventeen play suites on Windows, including live notifications, fragmented/mixed frames, queue frame/byte limits, deadlines, RPC rollback/restart and prior stop regressions. Output is build/core/play. Earlier console-process diagnostics are historical. POSIX remains unverified; see development.md.
