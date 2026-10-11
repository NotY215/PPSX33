Phase F status:
- ppu_runtime_run.h has Phase F null-table skip (ON main)
- ppu_runtime.h is a thin include wrapper
- ppu_runtime_core.h / _a / _b need to be present for build

If build fails on missing core, the previous working single-file ppu_runtime.h
is in git history at commit 730cc349 (Phase E).

To restore quickly:
  git show 730cc349:runtime/ppu_runtime.h > runtime/ppu_runtime.h

Then rebuild.
