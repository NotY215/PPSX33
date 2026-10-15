# Phase XXII — Pipeline correctness

| ID | Status | Work |
|----|--------|------|
| A–J | DONE | See prior drops |
| **XXII-K** | DONE | Save / pad surface tags |
| **XXII-L** | DONE | OpenGL probe prep (D3D11 default log) |

## Package cleanup (`project.cpp`)

After a successful build, `output/` keeps only:

- `build_live.log`
- `<game>.exe`
- `guest_image.bin`
- `pad_map.json`
- `ps3rt.dll`

`output/src`, `output/obj`, link rsp/pdb/exp removed. Reports copied to project root.  
`codebase/` compiler junk removed; sources kept.  
`input/` EBOOT kept; build noise removed.

## Window icon

Runtime loads **PIC1.PNG** (not ICON0.PNG) for HWND big/small icons.
