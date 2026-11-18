# Superseded Role Authorization Proposal

Superseded on 2026-11-17 by independent per-user bitmask permissions. Current policy and model are in `../session-permissions.md`. During the intermediate model-first stage RoleManager still backed Session/protocol; it has now been removed by the integrated permission change. Its retired API is archived in `2026-11-17-role-manager.md`. This proposal must not guide new permission checks.

The former hierarchy was Host > Master > Player > Spectator, with Any as a nonassignable threshold for anonymous pending peers. Host was immutable, new users were Players, only host could promote/demote Masters, and Masters could change other Players/Spectators between those two roles.

The old larger-game proposal mapped capabilities directly to roles:

| Capability | Game master | Player | Spectator |
| --- | --- | --- | --- |
| View and modify sheets | All sheets | Assigned sheets | Published information only |
| Import a personal sheet | Yes | Yes; importer initially controls it | No |
| Control characters | Explicit PC/NPC assignments | Explicit assignments, possibly multiple | No |
| Receive state | Master-authorized information | Own/assigned and shared information | Published information only |
| Administer membership | Role-based administrative policy | Own join/leave | Own join/leave |

These sheet semantics were proposals, not implemented permission flags. Several players controlling one sheet and retaining sheets when control is revoked remain requirements, but the new model must express access through concrete permissions and sheet bindings rather than preset names. Placeholder Permission1 through Permission5 do not yet implement those operations.
