# Buffered Stop Before the Correction

Archived 2026-11-12. The original communication review reproduced three times that the preliminary buffered-input loop could start a second peer's callback after stop was requested while the first callback ran. That loop did not check the stop flag; the later socket loop did. See [../session-flows.md](../session-flows.md) for current behavior.

The correction also covers a remote request blocked on ProtocolManager's execution mutex behind a local callback. Checking stop only before waiting for execution would allow that queued request to run after the local callback returned.

The two cases added to the existing admission suite fail against the earlier `any-commands` library and pass against the correction. Ignored `build/play/stop-buffered/stop_regression.cpp` selects each case independently for comparison. The original review probe remains historical and includes an obsolete command-name reproduction; it is not a current acceptance test.

The implemented policy finishes a request already admitted to execution, drops queued input/output during cleanup, joins the worker, and preserves identities, roles, tokens, and the SessionCode. It adds no forced cancellation, guaranteed final reply, shutdown event, or restriction on trusted local dispatch.
