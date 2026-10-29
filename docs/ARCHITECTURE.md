# PPSX33 Architecture

**Project site:** [https://ppsx33.xo.je/](https://ppsx33.xo.je/)  
**Repository:** [NotY215/PPSX33](https://github.com/NotY215/PPSX33)  
**Diagram index:** [DIAGRAMS.md](DIAGRAMS.md)

Intended architecture for the static recompiler and host runtime. Diagram sources use Mermaid, PlantUML, D2, Graphviz, seqdiag, blockdiag, Nomnoml, WaveDrom, Markmap, Svgbob, Vega-Lite, and C4 (see DIAGRAMS.md). Status labels are engineering targets, not playability claims.

## 1. System context

```mermaid
flowchart LR
  User[User with decrypted ELF] --> GUI[PS3Recomp GUI]
  GUI --> Core[ps3core lifter]
  Core --> Game[Generated game EXE]
  Game --> RT[ps3rt.dll]
  RT --> D3D[D3D11]
  Core --> MSVC[MSVC cl/link]
```

```plantuml
@startuml
actor User
User --> (Load ELF)
User --> (Lift)
User --> (Build)
(Build) --> (Game EXE)
(Game EXE) --> (ps3rt)
(ps3rt) --> (D3D11)
@enduml
```

## 2. Containers

```mermaid
flowchart TB
  subgraph HostTools
    GUI[PS3Recomp.exe]
    CORE[ps3core.dll]
    CLI[ps3_cli.exe]
  end
  subgraph Runtime
    RT[ps3rt.dll]
  end
  subgraph PerGame
    EXE[Game.exe]
    IMG[guest_image.bin]
  end
  GUI --> CORE
  CORE --> EXE
  EXE --> RT
  EXE --> IMG
```

```d2
gui: PS3Recomp.exe
core: ps3core.dll
rt: ps3rt.dll
game: Game EXE
gui -> core: lift/build
core -> game: emit
game -> rt: link/load
```

## 3. Pipeline

```mermaid
sequenceDiagram
  participant U as User
  participant G as GUI
  participant C as ps3core
  participant M as MSVC
  participant E as Game EXE
  participant R as ps3rt
  U->>G: Load decrypted ELF
  G->>C: Decompile / lift
  G->>M: Build project
  M->>E: Link with ps3rt.lib
  E->>R: Init + ppu_run
  R->>R: LV2 / PRX / RSX / SPU
```

```seqdiag
seqdiag {
  GUI -> ps3core [label = "lift"];
  GUI -> MSVC [label = "build"];
  MSVC -> Game [label = "link"];
  Game -> ps3rt [label = "run"];
}
```

## 4. Runtime layers

```mermaid
flowchart TB
  PPU[PPU chunks] --> MEM[Guest memory]
  PPU --> LV2[LV2 HLE]
  PPU --> PRX[PRX/NID]
  PRX --> LIB[cell* services]
  GCM[GCM FIFO] --> RSX[RSX decode]
  RSX --> D3D[D3D11 present]
  SPU[SPU/MFC] --> MEM
  LIB --> PAD[Pad]
  LIB --> AUD[Audio]
  LIB --> FS[Filesystem]
```

```blockdiag
blockdiag {
  group guest { PPU; MEM; FIFO; SPU; }
  group hle { LV2; PRX; }
  group host { D3D11; Audio; Pad; }
  PPU -> LV2; PPU -> PRX; FIFO -> D3D11;
}
```

```nomnoml
[PPU] -> [LV2]
[PPU] -> [PRX]
[FIFO] -> [RSX]
[RSX] -> [D3D11]
```

## 5. RSX path

```mermaid
flowchart LR
  PUT[Control PUT/GET] --> FIFO[FIFO RAM]
  FIFO --> DEC[Methods]
  DEC --> DRAW[Draw/Clear]
  DRAW --> PRESENT[Present]
  INJ[Boot assist] -.->|probe only| FIFO
```

**Invariant:** assist draws prove host path only; title PUT + shaders required for real scenes.

## 6. Trust boundaries

```mermaid
flowchart LR
  A[Lift count] -.->|≠| B[Correct semantics]
  C[methods>0] -.->|maybe| D[Inject]
  E[Visible window] -.->|≠| F[Title mesh]
```

```wavedrom
{ signal: [
  { name: "PC", wave: "x3.4.5.x", data: ["chunk","import","recover"] },
  { name: "PUT", wave: "0.1..1.0" },
  { name: "flip", wave: "0.1.0.1.0" }
]}
```

## 7. Build artifacts

| Artifact | Role |
| --- | --- |
| PS3Recomp.exe | GUI |
| ps3core.dll | ELF / lift / project |
| ps3rt.dll | Runtime HLE + RSX |
| ppsx33-logo.ico | App + UI + optional game icon |
| guest_image.bin | Guest memory |

```dot
digraph {
  gui -> core; core -> game; game -> rt;
}
```

```markmap
# PPSX33
## Tools
### GUI
### ps3core
## Runtime
### PPU
### RSX
### SPU
## Output
### Game EXE
```

```svgbob
 ELF --> Lifter --> EXE
                 |
                 v
               ps3rt
```

## 8. Related

- [https://ppsx33.xo.je/](https://ppsx33.xo.je/)
- [ROADMAP.md](../ROADMAP.md)
- [DIAGRAMS.md](DIAGRAMS.md)
- [ENGINEERING.md](../ENGINEERING.md)
- [README.md](../README.md)
