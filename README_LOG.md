# Run log files

When the game EXE starts, `ps3rt_log.cpp` creates:

```
<dir-of-GOW_3.exe>/log_YYYY-MM-DD_HH-MM-SS.txt
```

Example: `E:\PPSX33\build\dist\projects\GOW_3\output\log_2026-10-12_14-30-45.txt`

All `ps3rt_log(...)` lines (and after wiring, former stderr runtime logs) are written to both the console and this file.

**If build is broken:** `src/core/project.cpp` and `runtime/ps3rt.cpp` must be restored from commit `a5b2b6e` / `cc9b09d` and the copy list must include `ps3rt_log.cpp`.
