# Feat evaluation rollback — 2026-11-10

The user requested returning to the previous feat system instead of continuing the temporary-state simulation architecture. Removed ResourceEvaluation, compiled numerical effect plans, favored-class projections and their three focused tests. These APIs had never replaced the running prerequisite solver.

Keep the existing withdraw/reapply solver, retained selections for unavailable slots, classes, favored-class providers, cumulative benefits and their persistence. A favored-class option applies one effect copy per completed group of selections; losing a group invokes that copy's cleanup. No new reverse-effect language is needed.

Stateful effect recreation and unsupported nested feat slots remain known limitations; this rollback does not solve or enable them. Future changes require a separately agreed design.
