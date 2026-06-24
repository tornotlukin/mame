# MAME Debugger Command Reference for LLM

You are controlling the MAME debugger via a TCP connection. You issue commands exactly as they would be typed into the MAME debugger console. This document is the authoritative reference for command syntax. **Do not guess syntax. Do not use GDB, x86, or any other debugger syntax. Only use the exact syntax described here.**

---

## CRITICAL RULES

1. **Numbers are HEXADECIMAL by default.** `1234` means hex 1234 (not decimal). For decimal, prefix with `#` (e.g., `#100` = 100 decimal). For binary, use `0b`. For octal, use `0o`.
2. **There is NO `0x` prefix required** — bare numbers are already hex. `0x1234` works but is redundant. `1234` and `0x1234` are identical.
3. **Commas separate parameters**, not spaces (except within expressions). Example: `bp 1234,a0==0` NOT `bp 1234 a0==0`.
4. **Braces `{ }` are required** around action strings that contain commas or semicolons. Example: `bp 1234,1,{ printf "hit!\\n" ; g }`
5. **There is no `break` command.** Use `bp` (breakpoint). There is no `b` shortcut for breakpoint.
6. **There is no `x` command** (no GDB-style examine). Use `dump` for hex dumps or memory access operators (`b@`, `w@`, `d@`, `q@`) in expressions.
7. **There is no `info` command.** Use `bplist`, `wplist`, `rplist`, `symlist`, etc.
8. **There is no `delete` command.** Use `bpclear`, `wpclear`, etc.
9. **There is no `run` command.** Use `g` (go) to resume execution.
10. **There is no `next` for single-stepping.** `next` resumes until a different CPU is scheduled. Use `s` (step) or `o` (over) instead.
11. **Register names are CPU-specific.** A Z80 has `A`, `BC`, `DE`, `HL`, `PC`, `SP`. A 6502 has `A`, `X`, `Y`, `PC`, `SP`, `P`. A 68000 has `D0`-`D7`, `A0`-`A7`, `PC`, `SR`. Use `symlist` to discover available register names for the current CPU.
12. **There is no `set` command.** To modify a register, use `do`: `do pc=1234` or `do a0=ff`.
13. **Watchpoint syntax requires length AND type.** `wp 1234,1,w` — the `,1,w` is mandatory (length=1, type=write).

---

## EXECUTION COMMANDS

| Command | Syntax | Description |
|---------|--------|-------------|
| **step** | `s [<count>]` | Single-step `count` instructions (default: 1) |
| **over** | `o [<count>]` | Step over subroutine calls |
| **out** | `out` | Step out of current subroutine |
| **go** | `g [<address>]` | Resume execution. Optional address sets temp breakpoint |
| **go vblank** | `gv` | Resume until vertical blank |
| **go interrupt** | `gi [<irqline>]` | Resume until interrupt |
| **go exception** | `ge [<exception>[,<condition>]]` | Resume until exception |
| **go time** | `gtime <milliseconds>` | Resume for N milliseconds of emulated time. Use `#` for decimal: `gtime #1000` |
| **go branch true** | `gbt [<condition>]` | Resume until next taken branch |
| **go branch false** | `gbf [<condition>]` | Resume until next not-taken branch |
| **go next inst** | `gni [<count>]` | Resume with temp breakpoint `count` instructions ahead |
| **next CPU** | `n` | Resume until a different CPU is scheduled |
| **focus** | `focus <cpu>` | Only debug this CPU, ignore all others |
| **ignore** | `ignore <cpu>[,<cpu>...]` | Stop debugging specified CPUs |
| **observe** | `observe <cpu>[,<cpu>...]` | Resume debugging specified CPUs |

### Examples
```
s               -- step 1 instruction
s 10            -- step 16 (0x10) instructions. Remember: hex by default!
s #10           -- step 10 (decimal) instructions
o               -- step over 1 instruction
g               -- resume execution
g 1234          -- resume, stop at address 0x1234
gv              -- resume until vblank
gtime #5000     -- resume for 5 seconds of emulated time
```

---

## BREAKPOINT COMMANDS

| Command | Syntax | Description |
|---------|--------|-------------|
| **set** | `bp <address>[:<cpu>][,<condition>[,<action>]]` | Set breakpoint |
| **clear** | `bpclear [<id>,...]` | Clear breakpoint(s). No args = clear all |
| **disable** | `bpdisable [<id>,...]` | Disable breakpoint(s) |
| **enable** | `bpenable [<id>,...]` | Enable breakpoint(s) |
| **list** | `bplist [<cpu>]` | List breakpoints |

### Examples
```
bp 1234                              -- break at address 0x1234
bp 1234,a==0                         -- break at 0x1234 only when register A is 0
bp 1234:audiocpu                     -- break on the audio CPU at 0x1234
bp 1234,1,{ printf "PC=%04X\\n",pc ; g }  -- break, print, and auto-continue
bpclear 3                            -- clear breakpoint #3
bpclear                              -- clear ALL breakpoints
bplist                               -- list all breakpoints
```

**WRONG:** `break 1234`, `b 1234`, `bp set 1234`, `delete 3`
**RIGHT:** `bp 1234`, `bpclear 3`

---

## WATCHPOINT COMMANDS

Watchpoints trigger on memory access (read/write).

| Command | Syntax | Description |
|---------|--------|-------------|
| **set** | `wp <address>[:<space>],<length>,<type>[,<condition>[,<action>]]` | Set watchpoint |
| **set (data)** | `wpd <address>,<length>,<type>[,<cond>[,<action>]]` | Watchpoint on data space |
| **set (I/O)** | `wpi <address>,<length>,<type>[,<cond>[,<action>]]` | Watchpoint on I/O space |
| **clear** | `wpclear [<id>,...]` | Clear watchpoint(s) |
| **disable** | `wpdisable [<id>,...]` | Disable watchpoint(s) |
| **enable** | `wpenable [<id>,...]` | Enable watchpoint(s) |
| **list** | `wplist [<cpu>]` | List watchpoints |

**Type parameter:** `r` = read, `w` = write, `rw` = both

**Special variables in conditions:**
- `wpaddr` = the address that triggered the watchpoint
- `wpdata` = the data being written (write watchpoints only)

### Examples
```
wp 1234,1,w                          -- watch for writes to address 0x1234 (1 byte)
wp 1234,4,rw                         -- watch reads AND writes to 0x1234-0x1237
wp 1234,1,w,wpdata==0                -- only trigger when writing 0
wp 2000,100,w,1,{ printf "Write %02X to %04X\\n",wpdata,wpaddr ; g }
wpclear 5                            -- clear watchpoint #5
```

**WRONG:** `watch 1234`, `wp 1234 w`, `wp 1234,w` (missing length!)
**RIGHT:** `wp 1234,1,w`

---

## REGISTERPOINT COMMANDS

Registerpoints trigger when a condition on register values becomes true.

| Command | Syntax | Description |
|---------|--------|-------------|
| **set** | `rp <condition>[,<action>]` | Set registerpoint |
| **clear** | `rpclear [<id>,...]` | Clear registerpoint(s) |
| **disable** | `rpdisable [<id>,...]` | Disable |
| **enable** | `rpenable [<id>,...]` | Enable |
| **list** | `rplist [<cpu>]` | List registerpoints |

### Examples
```
rp a==ff                             -- break when register A equals 0xFF
rp pc==1234&&a==0                    -- break when PC=0x1234 and A=0
```

---

## EXCEPTIONPOINT COMMANDS

| Command | Syntax | Description |
|---------|--------|-------------|
| **set** | `ep <type>[,<condition>[,<action>]]` | Set exceptionpoint |
| **clear** | `epclear [<id>,...]` | Clear |
| **disable** | `epdisable [<id>,...]` | Disable |
| **enable** | `epenable [<id>,...]` | Enable |
| **list** | `eplist` | List exceptionpoints |

---

## MEMORY COMMANDS

| Command | Syntax | Description |
|---------|--------|-------------|
| **dump** | `dump <file>,<address>[:<space>],<length>[,<group>[,<ascii>[,<rowsize>]]]` | Dump memory to text file |
| **save** | `save <file>,<address>[:<space>],<length>` | Save raw binary to file |
| **load** | `load <file>,<address>[:<space>][,<length>]` | Load raw binary from file |
| **find** | `find <address>[:<space>],<length>[,<data>,...]` | Search memory for data pattern |
| **fill** | `fill <address>[:<space>],<length>[,<data>,...]` | Fill memory with pattern |
| **dasm** | `dasm <file>,<address>,<length>[,<opcodes>[,<cpu>]]` | Disassemble to file |
| **map** | `map <address>[:<space>]` | Map logical→physical address |
| **memdump** | `memdump [<file>]` | Dump memory maps to file |

### Data size prefixes for find/fill
- `b.` = byte, `w.` = word (16-bit), `d.` = dword (32-bit), `q.` = qword (64-bit)
- Wildcard: `?` matches any value

### Examples
```
dump ram.txt,0,1000                  -- dump 0x0000-0x0FFF to ram.txt
dump ram.txt,0,100,1,1,16            -- dump with 1-byte groups, ASCII, 16 bytes/row
save rom.bin,0,8000                  -- save raw binary 0x0000-0x7FFF
load patch.bin,1000                  -- load binary file into memory at 0x1000
find 0,10000,"HIGH SCORE"           -- search for ASCII string
find 0,10000,w.1234,w.5678          -- search for two consecutive 16-bit words
find 0,10000,b.03                   -- search for byte value 03
fill 2000,100,0                     -- fill 0x2000-0x20FF with zeros
fill 2000,10,b.ff                   -- fill 16 bytes with 0xFF
dasm game.asm,0,1000                -- disassemble 0x0000-0x0FFF to file
map 8000                            -- show what's mapped at address 0x8000
```

**WRONG:** `x/16xb 0x1000` (GDB syntax), `disassemble 0x1000` (GDB syntax)
**RIGHT:** `dump mem.txt,1000,100`, `dasm out.asm,1000,100`

---

## MEMORY ACCESS IN EXPRESSIONS

Use these operators to read/write memory in expressions, conditions, and the `do` command.

| Operator | Size | Side Effects | Example |
|----------|------|-------------|---------|
| `b@<addr>` | byte (8-bit) | suppressed | `b@1234` |
| `w@<addr>` | word (16-bit) | suppressed | `w@1234` |
| `d@<addr>` | dword (32-bit) | suppressed | `d@1234` |
| `q@<addr>` | qword (64-bit) | suppressed | `q@1234` |
| `b!<addr>` | byte (8-bit) | enabled | `b!1234` |
| `w!<addr>` | word (16-bit) | enabled | `w!1234` |
| `d!<addr>` | dword (32-bit) | enabled | `d!1234` |
| `q!<addr>` | qword (64-bit) | enabled | `q!1234` |

### Address space prefixes (prepend before size)
- `p` or `lp` = logical program (default)
- `d` or `ld` = logical data
- `i` or `li` = logical I/O
- `pp` = physical program
- `r` = direct program pointer
- `o` = direct opcode pointer
- `m` = memory region (requires tag prefix)
- `s` = memory share (requires tag prefix)

### Examples
```
print b@1234                         -- print byte at 0x1234
print w@1234                         -- print 16-bit word at 0x1234
do b@1234=ff                         -- write 0xFF to address 0x1234
do w@2000=1234                       -- write 0x1234 (16-bit) to address 0x2000
print dw@300                         -- print word at 0x300 in data space
bp 1000,b@40==3                      -- break at 0x1000 when byte at 0x40 equals 3
```

### Writing memory: use `do` with memory operators
```
do b@1234=ff                         -- write byte
do w@1234=abcd                       -- write word
do d@1234=12345678                   -- write dword
```

**WRONG:** `set *0x1234 = 0xff` (GDB), `memory write 0x1234 0xff`
**RIGHT:** `do b@1234=ff`

---

## GENERAL / UTILITY COMMANDS

| Command | Syntax | Description |
|---------|--------|-------------|
| **print** | `print <expr>[,<expr>...]` | Print expression value(s) as hex |
| **printf** | `printf "<format>"[,<args>...]` | C-style formatted print. Supports `%d`, `%x`, `%X`, `%s`, `%c`, `%o` |
| **do** | `do <expression>` | Evaluate expression (use for assignments) |
| **symlist** | `symlist [<cpu>]` | List all symbols/registers for a CPU |
| **history** | `history [<cpu>[,<length>]]` | Show recently executed PC addresses |
| **trackpc** | `trackpc [<enable>[,<cpu>[,<clear>]]]` | Enable/disable PC tracking |
| **trackmem** | `trackmem [<enable>[,<cpu>[,<clear>]]]` | Track which PC writes to each address |
| **pcatmem** | `pcatmem <address>` | Show which PC last wrote to an address |
| **softreset** | `softreset` | Soft reset the emulated system |
| **hardreset** | `hardreset` | Hard reset the emulated system |
| **statesave** | `ss <filename>` | Save state to file |
| **stateload** | `sl <filename>` | Load state from file |
| **rewind** | `rw` | Load most recent save state (undo) |
| **snap** | `snap [<filename>]` | Screenshot |
| **source** | `source <filename>` | Execute debugger script from file |
| **time** | `time` | Print emulated time elapsed |
| **quit** | `quit` | Exit MAME immediately |

### Examples
```
print pc                             -- print current program counter
print a,b,c                         -- print registers A, B, C
printf "A=%02X X=%02X\\n",a,x        -- formatted output
do pc=1234                           -- set PC to 0x1234
do a=ff                              -- set register A to 0xFF
symlist                              -- list all available registers/symbols
history                              -- show recent PC history
pcatmem 2000                         -- who last wrote to address 0x2000
```

**WRONG:** `set $pc = 0x1234` (GDB), `info registers` (GDB), `p/x $eax` (GDB)
**RIGHT:** `do pc=1234`, `symlist`, `print pc`

---

## TRACING

| Command | Syntax | Description |
|---------|--------|-------------|
| **trace** | `trace <file>\|off[,<cpu>[,<flags>[,<action>]]]` | Start/stop instruction tracing |
| **traceover** | `traceover <file>\|off[,<cpu>[,<flags>[,<action>]]]` | Trace, skipping subroutines |
| **traceflush** | `traceflush` | Flush trace files to disk |
| **tracelog** | `tracelog "<format>"[,<args>...]` | Log to trace file (use in actions) |
| **tracesym** | `tracesym <symbol>[,<symbol>...]` | Log symbols to trace file |

**Flags:** `noloop` (don't collapse loops), `logerror` (include error log). Combine with `|`: `noloop|logerror`

### Examples
```
trace game.tr                        -- trace execution to file
trace game.tr,,noloop                -- trace without loop detection
trace game.tr,,,{ tracelog "A=%02X ",a }  -- trace with extra logging
trace off                            -- stop tracing
```

---

## CHEAT SEARCH

The cheat search helps find memory locations that hold specific game values (lives, score, etc.).

| Command | Syntax | Description |
|---------|--------|-------------|
| **init** | `cheatinit [<sign><width>[<swap>]][,<addr>,<len>[,<space>]]` | Initialize search |
| **range** | `cheatrange <address>,<length>` | Add memory range to search |
| **next** | `cheatnext <condition>[,<value>]` | Filter by comparing to PREVIOUS values |
| **nextf** | `cheatnextf <condition>[,<value>]` | Filter by comparing to INITIAL values |
| **list** | `cheatlist [<file>]` | Show/save remaining candidates |
| **undo** | `cheatundo` | Undo last filter |

**Init format:** `<sign>` = `u`(unsigned) or `s`(signed), `<width>` = `b`(8) `w`(16) `d`(32) `q`(64), `<swap>` = `s`(byte-swap)

**Conditions for cheatnext/cheatnextf:**
| Condition | Aliases | Description |
|-----------|---------|-------------|
| `all` | | Update values, no filtering |
| `equal` | `eq` | Equal to previous (or to `<value>`) |
| `notequal` | `ne` | Not equal to previous (or to `<value>`) |
| `decrease` | `de`, `-` | Decreased (or decreased by `<value>`) |
| `increase` | `in`, `+` | Increased (or increased by `<value>`) |
| `decreaseorequal` | `deeq` | Decreased or unchanged |
| `increaseorequal` | `ineq` | Increased or unchanged |
| `smallerof` | `lt`, `<` | Less than `<value>` (required) |
| `greaterof` | `gt`, `>` | Greater than `<value>` (required) |
| `changedby` | `ch`, `~` | Changed by exactly `<value>` (required) |

### Complete Example: Finding Lives Counter
```
-- 1. Start game, have 3 lives. Break into debugger.
cheatinit ub                         -- initialize: unsigned byte search
-- 2. Resume game (g), lose a life, break into debugger.
cheatnext -,1                        -- filter: decreased by 1
-- 3. Resume (g), lose another life, break again.
cheatnext -,1                        -- filter again: decreased by 1
cheatlist                            -- show remaining candidates
-- 4. If one result: that's your lives address!
-- To make infinite lives: do b@<address>=3
```

**Abbreviations:** `ci` = cheatinit, `cr` = cheatrange, `cn` = cheatnext, `cnf` = cheatnextf, `cl` = cheatlist, `cu` = cheatundo

---

## CODE ANNOTATIONS

| Command | Syntax | Description |
|---------|--------|-------------|
| **add comment** | `comadd <address>,<comment>` or `// <address>,<comment>` | Add comment at address |
| **delete comment** | `comdelete <address>` | Remove comment |
| **save comments** | `comsave` | Save comments to XML |
| **list comments** | `comlist` | Load and display comments |
| **commit** | `commit <address>,<comment>` or `/* <address>,<comment>` | Add + save in one step |

---

## DEVICE/CPU SPECIFICATION

Many commands accept a CPU/device specifier:

| Format | Meaning |
|--------|---------|
| `maincpu` | Device with absolute tag `:maincpu` |
| `audiocpu` | Device with absolute tag `:audiocpu` |
| `0`, `1`, `2` | CPU by zero-based index |
| `.` | The currently visible CPU |
| `^sibling` | Sibling device relative to visible CPU |
| `.:child` | Child device of visible CPU |

### Address space suffixes (for memory commands)
Append `d`, `i`, or `o` to commands to target data/IO/opcode spaces:
- `findd` = find in data space
- `dumpi` = dump from I/O space
- `saveo` = save from opcode space

Or specify explicitly: `dump file.txt,1234:data,100`

---

## EXPRESSION SYNTAX SUMMARY

- **All values are unsigned 64-bit**
- **Operators:** `+ - * / % << >> & | ^ ~ ! && || == != < > <= >= = ++ --`
- **Assignment:** `=` (and `+=`, `-=`, `*=`, `/=`, `%=`, `<<=`, `>>=`, `&=`, `|=`, `^=`)
- **Functions:** `min(a,b)`, `max(a,b)`, `if(cond,true,false)`, `abs(x)`, `bit(x,n[,w])`, `s8(x)`, `s16(x)`, `s32(x)`
- **Number formats:** `1234` (hex), `$1234` (hex), `0x1234` (hex), `#1234` (decimal), `0b1001` (binary), `0o777` (octal)
- **Temp variables:** `temp0` through `temp9` — persistent across commands within a session

---

## COMMON PATTERNS

### "What does the code at address X do?"
```
dasm output.asm,<address>,100        -- disassemble 0x100 bytes to file
```

### "What is at memory address X?"
```
print b@<address>                    -- read 1 byte
print w@<address>                    -- read 2 bytes (word)
dump mem.txt,<address>,100           -- hex dump 0x100 bytes
```

### "Write a value to memory"
```
do b@<address>=<value>               -- write byte
do w@<address>=<value>               -- write word
```

### "Break when address X is written to"
```
wp <address>,1,w                     -- watchpoint on 1-byte write
```

### "Break when a specific value is written to address X"
```
wp <address>,1,w,wpdata==<value>     -- conditional watchpoint
```

### "Find a string in memory"
```
find 0,10000,"TEXT"                  -- search for ASCII string
```

### "Find a byte value in memory"
```
find 0,10000,b.<value>              -- search for specific byte
```

### "Set a register"
```
do <register>=<value>                -- e.g., do pc=1234, do a=ff
```

### "Stop at address X, run a command, and auto-continue"
```
bp <address>,1,{ <commands> ; g }    -- the ; g resumes execution
```

---

## THINGS THAT DO NOT EXIST IN MAME DEBUGGER

Do NOT use any of these — they are from other debuggers and will produce errors:

| Wrong (GDB/LLDB/x86) | Right (MAME) |
|----------------------|---------------|
| `break 0x1234` | `bp 1234` |
| `b 0x1234` | `bp 1234` |
| `delete 3` | `bpclear 3` |
| `info breakpoints` | `bplist` |
| `info registers` | `symlist` or `print pc,a,x` |
| `run` | `g` |
| `continue` | `g` |
| `c` | `g` |
| `next` (step over) | `o` |
| `nexti` | `o` |
| `stepi` | `s` |
| `finish` | `out` |
| `x/16xb 0x1000` | `dump file.txt,1000,10` |
| `disassemble 0x1000` | `dasm file.asm,1000,100` |
| `set $pc = 0x1234` | `do pc=1234` |
| `set *0x1234 = 0xff` | `do b@1234=ff` |
| `p/x $eax` | `print eax` |
| `watch *0x1234` | `wp 1234,1,rw` |
| `display` | *(no equivalent — use printf in actions)* |
| `bt` / `backtrace` | `history` |
