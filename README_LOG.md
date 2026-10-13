# Runtime log files

PPSX33 includes runtime logging in `runtime/ps3rt_log.cpp`. The runtime log helper is part of the runtime sources copied into a project's staged build. When a generated game executable starts, the log destination is associated with the executable's directory and uses a timestamped filename of the form:

```text
<game-executable-directory>/log_YYYY-MM-DD_HH-MM-SS.txt
```

For example, a run from `build/dist/projects/<name>/output/` should place its log alongside that executable when the runtime logging path is enabled. Exact log contents depend on which runtime paths emit messages. Keep the Console output and generated log together when reporting a runtime failure.

## Troubleshooting

- Confirm that `runtime/ps3rt_log.cpp` is present in the runtime source tree and included by the current build/staging logic.
- Preserve the exact PPSX33 commit, game executable revision, selected graphics backend, and log when reproducing a failure.
- Do not use a successful build or the existence of a log file as evidence that a commercial game is playable.
