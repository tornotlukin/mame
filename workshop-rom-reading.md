# Workshop: CPS / CPS2 / CPS3 ROM Reading Skill

**Started:** 2026-06-21
**Status:** ✅ Built — skill created & tested

---

## The Idea

A Claude Code skill that lets the LLM genuinely understand how Capcom CPS, CPS2, and
CPS3 ROMs work. It treats the MAME source tree as ground truth (the per-game
`ROM_REGION`, `GFXDECODE`, memory-map, and encryption definitions already encode exactly
how each game is organized) and combines that static knowledge with the live MCP
debugger to read decrypted memory, disassemble code, and dump graphics from a running
game. The user supplies contextual clues (genre, what they're hunting for) and the skill
fuses those with the ROM facts to answer questions or produce a structured map.

---

## Ground Truth Found in MAME (the source of accuracy)

Located under `src/mame/capcom/`:

| File | Lines | What it gives us |
|------|-------|------------------|
| `cps1.cpp` | 15,205 | CPS1 ROM definitions, memory maps, GFXDECODE layouts |
| `cps1_v.cpp` | 3,070 | CPS1 video: tilemaps, sprites, palette |
| `cps1.h` | 372 | CPS1 device/state declarations |
| `cps2.cpp` | 12,902 | CPS2 driver, ROM defs, memory maps |
| `cps2crypt.cpp` | 713 | CPS2 encryption scheme (decryption keys/algorithm) |
| `cps3.cpp` | 4,461 | CPS3 driver |
| `cps3_a.cpp` | — | CPS3 audio (custom DSP) |

Key per-game declarations the skill will parse:
- `ROM_REGION( size, "tag", flags )` — `maincpu` (68000 code), `gfx`, `audiocpu` (Z80),
  `oki` (samples), `stars`, `*plds` (PALs)
- `GFXDECODE_ENTRY` + `gfx_layout` — exact bit packing of graphics tiles (8x8, 16x16, 32x32)
- `*_map(address_map &)` — CPU memory maps
- CPS2: encryption (XOR/keys) — RAM is decrypted, so live debugger sees plaintext

---

## Answers & Decisions

- **Mode: Hybrid.** Static parse of MAME source + ROM files on disk, PLUS live MCP
  debugger (read_memory, disassemble, dump VRAM/palette) on a running game.
- **Goals: ALL of the following** —
  1. Map graphics / sprites (decode tiles, locate sprites, palettes)
  2. Find values / cheats (locate health/score/timers/lives in RAM via code + clues)
  3. Understand code (disassemble & explain 68000 / Z80 / SH-2)
  4. Explain ROM layout (structured map of every region)
- **Source of truth: Auto-parse per game.** On demand, grep the relevant `cps*.cpp`
  for the specific game's `ROM_REGION` / `GFXDECODE` / memory_map and build the map
  from that — accurate and covers any game, not a static summary.

---

## Open Questions

- (resolved) Output → **Both**: live answers + persistent `docs/roms/<game>.md`.
- (resolved) Clues → **Guided hunt**: hypothesis from code/genre, then drive the live
  debugger to confirm.
- (resolved) Packaging → **Full skill `/cps-rom`** with reference docs + helper scripts.
- Remaining: exact slash-command name (`cps-rom` proposed) and whether helper parser is
  Python (matches `bridge/`) or pure-grep.

---

## Locations Config — `.claude/mame-rom-read-loc.md`

On every run the skill first reads `.claude/mame-rom-read-loc.md` (project folder) for the
three paths it needs. **If the file does not exist, the skill prompts the user for them
and creates it.** This makes the skill portable and self-configuring.

Stored paths:
1. **MAME codebase path** — the local MAME git (source of truth, `src/mame/capcom/...`).
2. **llm-debugger MAME path** — the installed/built MAME with the llm-debugger
   (the executable launched for live analysis).
3. **ROM folder target** — where the game ROM sets live.

(Mirrors the existing `.claude/github_url.md` prompt-and-save pattern.)

---

## The Five Inputs the Skill Fuses

The skill draws on exactly these sources every run:

1. **Local MAME git** (`src/mame/capcom/cps*.{cpp,h}`) — ground-truth ROM layout, gfx
   decode, memory maps, encryption. Parsed per game.
2. **Installed MAME + llm-debugger** — the live running emulator, driven via the MCP
   debugger tools (read_memory, disassemble, search_memory, watchpoints, screenshot).
   This is where encrypted CPS2/CPS3 data is seen decrypted.
3. **A game ROM** — the specific set being analyzed (matched to its MAME driver entry by
   ROM name/region tags).
4. **Documentation** — `docs/llm-debugger-reference.md`, MAME RST docs, plus the skill's
   own CPS reference docs.
5. **Prompting** — the user's contextual clues (genre, what to find, hypotheses) that
   steer the guided hunt.

---

## Proposed Skill Architecture (`/cps-rom`)

Location: `.claude/skills/cps-rom/`

```
.claude/skills/cps-rom/
  SKILL.md                  # entry: when to use, the workflow, the 5 inputs
  reference/
    cps1.md                 # CPS1 chip/ROM-region/gfx-layout primer (distilled from source)
    cps2.md                 # CPS2 + encryption notes (why live debugger is needed)
    cps3.md                 # CPS3 + SH-2 + custom audio
    workflows.md            # the guided-hunt loops per goal (gfx/cheat/code/layout)
  scripts/
    parse_driver.py         # extract a game's ROM_REGION/GFXDECODE/memory_map from cps*.cpp
                            # -> emits structured JSON the LLM consumes
```

**Workflow per run:**
0. Read `.claude/mame-rom-read-loc.md` for the 3 paths. If missing → prompt user, create it.
1. Identify the game + system (CPS1/2/3) → locate its driver block in `cps*.cpp`.
2. `parse_driver.py` extracts the static map (regions, sizes, chips, gfx layout, crypt).
3. Read/refresh `docs/roms/<game>.md` (persistent map) — reuse prior findings.
4. Take the user's clue → form a hypothesis grounded in the static map + reference docs.
5. Drive the live debugger to confirm (e.g. for a cheat: watchpoint + search_memory as
   the value changes; for gfx: dump tiles/palette; for code: disassemble around a PC).
6. Write confirmed findings back to `docs/roms/<game>.md` so knowledge compounds.

**Goal → static-vs-live lean:**
- ROM layout: mostly static (source parse) + ROM file sizes.
- Graphics/sprites: static layout defines packing; live dumps VRAM/palette to locate.
- Cheats/values: mostly live (search + watchpoints); code gives candidate addresses.
- Code understanding: live disassembly of decrypted code; source gives memory-map labels.

---

## Conversation Log

> **User:** Make a ROM reading skill specifically for Capcom CPS, CPS2, CPS3. Everything
> we need is in the MAME files (docs + cpp explaining chip organization). Can the LLM go
> through ROMs and really know how they work, with contextual clues from the user?

> **Claude:** Surveyed `src/mame/capcom/` to ground the design. Confirmed MAME already
> encodes per-game ROM organization (ROM_REGION, GFXDECODE, memory maps, CPS2 crypt).
> Asked 3 design questions.

> **User (answers):** Hybrid mode; all four goals (gfx, cheats, code, layout);
> auto-parse per game.

> **User (answers 2):** Output = Both (live + persistent map); Clues = Guided hunt
> (hypothesize then verify via debugger); Packaging = Full `/cps-rom` skill with scripts.

> **User:** The skill will use: the current MAME git locally, the installed MAME on the
> computer with the llm-debugger, a game ROM, any documentation, and prompting.

> **Claude:** Recorded as "The Five Inputs" and drafted the full skill architecture
> (`/cps-rom` with SKILL.md, reference/ docs per system, scripts/parse_driver.py, and the
> persistent docs/roms/<game>.md output). Ready to build pending sign-off.

> **User:** Skill should create `.claude/mame-rom-read-loc.md` holding paths to the MAME
> codebase, the llm-debugger MAME, and the ROM folder target. If it doesn't exist, prompt
> the user for those locations.

> **Claude:** Added the locations-config step as workflow step 0 (read-or-prompt-and-create),
> mirroring the github_url.md pattern.

---

## Key Insights

- MAME source is a *machine-readable spec* of each game's ROM layout — we don't need to
  reverse-engineer the format, we parse MAME's own definitions.
- CPS2/CPS3 are encrypted: static ROM bytes are scrambled, so the **live debugger is
  essential** to see real code/graphics. This makes Hybrid mode the only complete option.
- The existing LLM debugger MCP tools (read_memory, disassemble, search_memory,
  screenshot, cheat_*) are exactly the live-analysis primitives this skill needs.

---

## Build Result (2026-06-21)

> **Moved to user-global** `~/.claude/skills/cps-rom/` so every project can use it.
> Made portable: locations config now resolves project-local → user-global
> (`~/.claude/mame-rom-read-loc.md`); per-game maps + docs paths key off `<mame_codebase>`;
> parser invoked from the skill's own `<skill_dir>`. Project copy removed.

Skill created at `~/.claude/skills/cps-rom/`:
- `SKILL.md` — workflow, step-0 locations config, encryption warnings, and a
  **"Scripts are LIVING tools"** section (revise/extend scripts as ROMs reveal new info).
- `reference/cps1.md|cps2.md|cps3.md` — chips, ROM regions, gfx layouts, memory maps,
  work-RAM/cheat ranges, encryption notes — all distilled from the actual source.
- `reference/workflows.md` — guided-hunt loops for layout / cheats / gfx / code.
- `scripts/parse_driver.py` — parses a game's ROM_START block → JSON (regions, sizes,
  per-file offset/length/CRC/SHA1; resolves size macros like CODE_SIZE/QSOUND_SIZE).

Tested: `forgottn` (CPS1), `ssf2` (CPS2 — shows `key` region), `redearth` (CPS3 — shows
only `bios`, confirming CPS3's different SIMM/CD layout → a case for revising the parser),
and the not-found error path.

Decisions locked: name `cps-rom`; Python parser; output Both (live + `docs/roms/<game>.md`);
guided hunt; hybrid (static source + live llm-debugger).

## Next Steps (optional)

- First real run will prompt for and create `.claude/mame-rom-read-loc.md`.
- Extend `parse_driver.py` to follow CPS3 SIMM/device regions and clone→parent ROM
  inheritance when a real CPS3/clone analysis needs it (per the living-scripts principle).
- Commit the skill to the repo when ready.

---

## Resources & Links

- MAME CPS source: `src/mame/capcom/cps*.{cpp,h}`
- LLM debugger reference: `docs/llm-debugger-reference.md`
- MCP debugger tools: read_memory, disassemble, search_memory, get_registers,
  set/list breakpoints & watchpoints, cheat_init/next, screenshot, run_to_vblank
