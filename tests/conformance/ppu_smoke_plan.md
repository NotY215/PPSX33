# PPU smoke plan (first batch)

1. `addi` / `addis` / `add` with record forms  
2. `ld` / `std` / `lwz` / `stw` endian and update forms  
3. `bc` / `bclr` / `bcctr` LR/CTR behavior  
4. `cmp` / CR field moves  
5. One illegal encoding → must **not** be reported as FullyImplemented  

Until a host runner exists, use lift_report + manual register checks on tiny ELFs under `tests/`.
