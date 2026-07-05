# Workshop: MAME for Android (full controller access)

**Started:** 2026-07-05
**Status:** In Progress — design locked; hands-on phase starts when Android Studio is installed

---

## The Idea

Get a MAME build on Android that exposes **all** of a physical controller's buttons.
MAME4droid exists but the user dislikes that it doesn't give access to every controller
button. Core question: **fork MAME4droid and add full controller-button access**, or is
there a better path?

---

## Facts Established (research)

- The live project is **MAME4droid (Current)** — `github.com/seleuco/MAME4droid-Current`
  by David Valdeita (Seleuco), a port of *current* MAME (40,000+ ROMs). The older
  `MAME4droid-0.139u1` repo is the legacy version.
- Native emulator libraries live in a separate repo: `seleuco/MAME4droid_Native`.
- So **yes, source is available to fork** (MAME-derived → GPL-family licensing, forks are
  legitimate).
- Documented controller limitations in the issue tracker, e.g. **issue #104**: right
  analog stick recognized as buttons A/B/C/D (shared input IDs) on Retroid Pocket 6;
  recent releases improved controller auto-detect, right-stick true analog, and mapping UI.
- Architecture (typical): Java/Kotlin Android layer handles input (KeyEvent/MotionEvent)
  → maps to a fixed internal button set → passes to native MAME core via JNI. The
  "not all buttons" complaint usually lives in that Java input layer's fixed mapping
  table, NOT in the MAME core (desktop MAME maps everything).

---

## Open Questions

All major design questions are RESOLVED (see Answers & Decisions). Remaining items are
empirical — answered by inspection in Phase 0:

- How exactly does MAME4droid_Native's build consume MAME source (submodule? snapshot?
  patch set?) and how painful is pointing it at our 0.288 fork?
- Where exactly does input cross JNI in MAME4droid-Current, and does the shell expose
  MAME's TAB menu already (probably yes, via touch)?
- Does the shell code assume newer-than-0.288 core APIs anywhere?
- NDK toolchain requirements (MAME needs C++17+; current NDK is fine — verify versions).

---

## Answers & Decisions

- **Device: Retroid Pocket 6** (Android handheld, built-in controls incl. M1/M2 back
  buttons — user says those already map like regular buttons).
- **What's missing:** (1) right analog stick (misread as A/B/C/D buttons — this is exactly
  upstream issue #104, open & unresolved on RP6), (2) extra/special buttons, (3) the app
  exposes too few inputs to MAME overall ("not enough MAME inputs" for games that need more).
- **Controllers to support:** RP6 built-ins + Xbox-style/8BitDo + DualShock/DualSense +
  arcade sticks/encoders — i.e. the fix should be general HID handling, not RP6 hardcodes
  where avoidable.
- **Goal: personal build first**, then publish any Retroid Pocket 6-specific changes
  publicly (GitHub fork; possibly PR upstream since #104 is open).
- **Tooling:** user is installing Android Studio now (current version). Continue
  workshopping design before touching code.
- **Android SDK path: `H:\_DEV\android\SDK`** — verified present: build-tools, platforms,
  platform-tools (adb ✓), emulator. **NDK not installed yet** — required for Phase 2
  (native core build from our fork); install via SDK Manager when we get there.
- **Input model: PASS-THROUGH (desktop parity).** Forward every Android key/axis into
  MAME's own input system so the TAB menu can bind any button to any game input,
  per-game — not an extended fixed set.
- **Custom core: YES.** The endgame is building the native libs from the USER'S mame
  fork (jrpacman 4P now, CPS2 4P later) so the RP6 runs his mods. NDK build of the core
  is in scope.
- **Players: design for 8.** (X-Men is 6P; be safe at 8.) Docked RP6 = HDMI out, USB
  ports + hub, Bluetooth, ethernet — effectively a small computer. Multi-controller
  device→player routing is a first-class requirement, handled by MAME itself once each
  controller is a distinct input device.
- **Core version: PIN ANDROID TO 0.288** — build the Android core from the user's fork
  as-is (mods + llm-debugger unchanged); revisit only if MAME4droid_Native inspection
  shows the shell hard-requires a newer core API.
- **WiFi debugging: YES** — compile debugremote into the Android core so Claude/MCP on
  the PC can attach to games running on the RP6 over WiFi/ethernet (it's an OSD module
  + 3 build lines, per MCP/INTEGRATION.md).
- **Deployment:** RP6 is unlocked (not rooted) — plain APK install by copying the file
  (no root needed; adb kept for live logcat during input debugging).
- **Branding the fork: the app is named "MAMEalicious".** Custom app icon (replace
  `res/drawable/icon_foreground.png` + `icon_background.png`; adaptive icon via
  `mipmap-anydpi/ic_launcher.xml`), `app_name` = "MAMEalicious" in `strings.xml`, and
  `applicationId` = `com.tornotlukin.mamealicious` so it installs side-by-side with
  official MAME4droid (separate settings/paths, easy A/B input comparison).
- **Input polling/ground-truth: NO separate app needed** — it's plug-in-the-device work:
  `adb shell getevent -l` (raw kernel events, zero code), `adb shell dumpsys input`
  (device/axis inventory), plus Log.d instrumentation inside MAMEalicious's own
  InputHandler watched via `adb logcat`. Wireless adb (Android 11+) works too — no cable.

---

## Conversation Log

> **User:** Start a workshop on a MAME version for Android. MAME4droid exists but doesn't
> give access to all the controller's buttons. Can we fork MAME4droid and add that?

> **Claude:** Researched: MAME4droid-Current is actively developed, source on GitHub
> (app + separate native repo), forkable. Controller limitations are known issues (e.g.
> #104 right-stick=ABCD collision). The limitation likely lives in the Java input layer's
> fixed mapping, not the MAME core. Asked scoping questions.

> **User (answers):** Retroid Pocket 6; missing = right stick + special buttons + not
> enough MAME inputs; M1/M2 map like regular buttons; also uses Xbox/PS/arcade-stick
> controllers; goal = personal build, then publish RP6-specific changes.

> **Claude:** Confirmed #104 is the exact RP6 bug (open, unresolved, RetroArch works on
> same device → MAME4droid input layer at fault). Located input-code lineage
> (`input/InputHandler*.java`), found precedent fork (GeneralGamePadSupport). Drafted
> 4-phase plan: baseline build → fix mapping → widen Java→JNI input funnel → publish/PR.

> **User (design decisions):** Pass-through input (desktop parity); YES to his fork as
> the native core (jrpacman 4P → RP6); design for 8 players (X-Men 6P exists; docked RP6
> ≈ small computer: HDMI, USB, ethernet). Installing Android Studio meanwhile.

> **Claude:** Reframed project as "MAME4droid = thin Android shell around our fork."
> Revised plan: Phase 1 = pass-through OSD input module (fixes #104 by construction),
> Phase 2 = our core via MAME4droid_Native (+ possible debugremote-over-WiFi bonus),
> Phase 3 = publish/PR. Flagged version-alignment as the key open decision.

> **User (final decisions):** Pin Android core to 0.288 (our fork as-is); YES compile
> debugremote into the Android core (WiFi debugging of the RP6 from the PC). Android
> SDK installed at `H:\_DEV\android\SDK`.

> **Claude:** Design locked. Verified SDK present (adb ✓, NDK not yet — needed Phase 2).
> Remaining open items are empirical (Phase 0 inspection of the two MAME4droid repos).

> **User:** ok hit it

> **Claude:** Phase 0 executed: cloned both repos, mapped the whole input path. Headlines:
> the native core is "any MAME ≥0.261 + their OSD overlay" (our 0.288 fork qualifies);
> no prebuilt libs (NDK required even for stock APK); native OSD already registers real
> MAME devices w/ 6 axes; L3/R3 commented out in input.cpp; MYOSD_NUM_JOY=4 is the player
> cap; Java GameController's fixed table is the funnel. Work is far smaller than feared.

> **User:** Installed NDK 28.2.13676358 (+ NDK 30 also present); asked whether device
> Android version (RP6 = 13) requires matching SDK. Answer: no — compileSdk 36 / minSdk 29
> covers it; keep platform 36 installed; gradle pins NDK 28.2 so NDK 30 is ignored.

> **Claude:** Verified tooling complete. Discovered the two-stage native architecture:
> Gradle builds only a thin JNI shim that dlopens libMAME4droid.so (= MAME core + myosd
> OSD, built out-of-band from a MAME tree; no build glue ships — we write the
> scripts/src/osd/myosd.lua + NDK makefile glue in our fork, like we did for debugremote).

---

## Key Insights

- Forking is legally and practically viable; the input limitation is most likely in the
  Android (Java) input-handling layer, which is the smaller/easier half of the codebase.
- **Issue #104 = our exact bug**: RP6 right stick reads as Button C etc. Open, no dev
  response, no workaround. Same device works in RetroArch → definitively MAME4droid's
  input layer, not hardware/OS. Great first target and a ready-made upstream PR.
- Input code (by repo lineage): `com/seleuco/mame4droid/input/InputHandler.java` (+
  `InputHandlerExt.java` in older trees) — handles KeyEvent/MotionEvent → internal
  button values → JNI to the native core. Two distinct problems live here:
  1. **Mapping bugs/coverage** (right stick axes, special buttons) — pure Java fixes.
  2. **The funnel**: the app squeezes all input into a fixed virtual-pad button set
     before the core sees it. "All buttons" ultimately means widening the Java→JNI
     input surface (or passing raw inputs through) so MAME's own per-game input
     system (TAB menu) can bind anything — like desktop MAME does.
- **Precedent exists**: `catcatcatcatcatcat/MAME4droid-GeneralGamePadSupport` forked
  MAME4droid (0.139u1) specifically to generalize gamepad support — proof of pattern.
- Native libs are a separate repo (`MAME4droid_Native`); if fixes stay in the Java
  layer we likely don't need to rebuild the MAME core at all (app repo ships/consumes
  built libs) — hugely reduces build burden for a personal APK.

---

## The Reframe (from the three decisions)

The project is **"MAME4droid as a thin Android shell around OUR mame fork"**:

- MAME4droid's real value to us = the proven **Android OSD layer** (GL rendering, audio,
  touch overlay, storage/SAF handling, app lifecycle). Its input funnel is the part we
  replace.
- **Pass-through input done right**: implement/extend the Android OSD *input module* so
  each Android input device (built-ins, each BT/USB pad, arcade encoder) registers as a
  **distinct MAME input device with all its buttons/axes**. Then MAME's existing input
  system gives us the TAB menu, per-game binds, and 8-player device→player assignment
  for free — identical to desktop. (MAME's OSD is modular; our debugremote is already an
  OSD module, so this is familiar territory.)
- **Custom core**: `MAME4droid_Native` is the NDK build of the MAME core. Point it at our
  fork instead of stock MAME → jrpacman 4P (and future CPS2 4P) on the RP6.
- **Version alignment (open):** our fork is 0.288-based; MAME4droid-Current tracks the
  latest MAME. Either pin the Android native build to 0.288 (easy, matches our mods) or
  keep rebasing our mods forward. Decide after inspecting MAME4droid_Native's build setup.
- **Tantalizing side effect:** our debugremote module is a TCP OSD module — if it compiles
  into the Android core, the PC (Claude + MCP) could **live-debug games running on the
  RP6 over WiFi**. Docked RP6 even has ethernet.

## Phase 0 Findings (code inspection, 2026-07-05)

Repos cloned to `H:\_DEV\mame4droid\` (MAME4droid-Current + MAME4droid_Native).

**Architecture (better than hoped):**
- `MAME4droid-Current/src/` is an **OSD overlay** dropped into a *current MAME source
  tree* (howtobuild.txt: "Download 0.261 MAME or later, put files inside MAME src").
  → The native core = stock MAME + `src/osd/myosd/`. **Our 0.288 fork qualifies** — drop
  the overlay into our tree, build with NDK, our mods + debugremote ride along.
- **`MAME4droid_Native` repo is the LEGACY 0.139u1 build — irrelevant for Current.**
- **No prebuilt libs ship** (`app/src/main/jniLibs/arm64-v8a/` is empty) → NDK build of
  `libmame4droid-jni.so` is required even for a stock APK. NDK must be installed first.
- App: standard Gradle project (`android-MAME4droid/`, gradlew wrapper, arm64-v8a only).

**The input funnel, precisely located:**
- **JNI surface** (`Emulator.java`): `setDigitalData(player, long bitmask)` +
  `setAnalogData(type, player, v1, v2)` + `setKeyData(keyCode,...)` (raw keyboard
  channel already exists!).
- **Java side** (`input/GameController.java`): fixed `emulatorInputValues[]` table;
  `keyMapping[emulatorInputValues.length * 4]` → **4 players hardcoded**.
- **Native side** (`src/osd/myosd/input.cpp`, 885 lines): GOOD NEWS — it already
  registers real MAME input devices per joystick with 6 axes (LX/LY/LZ/RX/RY/RZ) and
  buttons A–H (=L1/R1/L2/R2), Select/Start, D-pad. MAME's TAB menu therefore already
  sees these devices!
- **`MYOSD_NUM_JOY = 4`** (`myosd_core.h:41`) — the 4-player cap, one #define + loops.
- **L3/R3 are LITERALLY COMMENTED OUT** (`input.cpp:172-173`) — the user's missing
  buttons, sitting there disabled. Uncomment + wire Java side = quick win.
- Per-player state is a 64-bit bitmask (`long`) — plenty of headroom for more buttons.

**Build architecture (confirmed by inspection):**
- The APK contains a **thin JNI shim** (`app/src/main/jni/mame4droid-jni.c`, built by
  Gradle/ndkBuild) that at runtime **dlopen()s `libMAME4droid.so`** and resolves
  `myosd_droid_*` entry points (main, setVideoCallbacks, setDigitalData, setAnalogData…).
- **`libMAME4droid.so` = the entire MAME core + myosd OSD**, built OUT-OF-BAND from a
  MAME source tree with the overlay applied, then dropped into
  `app/src/main/jniLibs/arm64-v8a/`. The repo ships NO build glue for it (howtobuild:
  "modify makefile/lua scripts so use new OSD" — DIY).
- So our Phase-2 core build = in OUR fork: add `scripts/src/osd/myosd.lua` (+ makefile
  glue) selecting the myosd OSD, cross-compile with NDK r28c → libMAME4droid.so.
  Structurally identical to how we added debugremote to modules.lua — familiar work.
- Entry points to preserve: everything `mame4droid-jni.c` dlsym's (`myosd_droid_*`).
- **Tooling verified:** SDK platform 36 ✓, build-tools ✓, adb ✓, NDK 28.2.13676358 (r28c)
  ✓ + NDK 30 side-by-side (harmless; gradle pins 28.2). SDK ver ≠ device ver: compileSdk
  36 APK runs on RP6's Android 13 (API 33 ≥ minSdk 29). No API-33 SDK needed.

**RP6 INPUT GROUND TRUTH (live capture via adb getevent, 2026-07-05):**

Device: `/dev/input/event7` "Retroid Pocket Controller" (single kernel device for all
built-ins). Confirmed by cued live capture:

| Physical control | Kernel event | Notes |
|------------------|--------------|-------|
| **M1 (back)** | **`BTN_C`** | regular button, confirmed by press |
| **M2 (back)** | **`BTN_Z`** | regular button, confirmed by press |
| Right stick | **`ABS_Z` + `ABS_RZ`** | true analog ±32767 — Xbox-style axis pair |
| R3 / L3 | `BTN_THUMBR` / `BTN_THUMBL` | present & working at kernel level |
| L2 / R2 analog | `ABS_BRAKE` / `ABS_GAS` | 0..32767 |
| L2 / R2 click | `BTN_TL2` / `BTN_TR2` | digital, fires at full pull |
| L1 / R1 | `BTN_TL` / `BTN_TR` | |
| Face A/B/X/Y | `BTN_GAMEPAD(A)`, `BTN_EAST`, `BTN_NORTH`, `BTN_WEST` | |
| D-pad | `BTN_DPAD_*` **and** `ABS_HAT0X/Y` | dual-reported |
| Select/Start/Home | `BTN_SELECT`, `BTN_START`, `BTN_MODE` | + `KEY_HOME/BACK/APPSELECT/F24` system keys |
| Left stick | `ABS_X` + `ABS_Y` | analog ±32767 |

**#104 mechanism confirmed at kernel level:** the right stick is a real analog pair on
`ABS_Z/ABS_RZ` — the same axes many gamepads use for *triggers* (RP6 triggers are on
GAS/BRAKE instead). Android delivers these as `AXIS_Z`/`AXIS_RZ` MotionEvents;
MAME4droid's Java layer evidently treats Z/RZ as trigger/button values → phantom
A/B/C/D presses. Fix: classify Z/RZ as right-stick axes (as the native `input.cpp`
already expects — it has RX/RY items fed from `MYOSD_AXIS_RX/RY`).

**Everything the user wants is present at the kernel level** — M1/M2 are ordinary
buttons (`BTN_C`/`BTN_Z` → Android `KEYCODE_BUTTON_C`/`KEYCODE_BUTTON_Z`), L3/R3 exist,
right stick is true analog. All losses happen in the app layer. Polling method: adb
getevent (live stream), dumpsys input, logcat — no separate app needed.

**Revised understanding of the work:**
1. Quick wins: uncomment L3/R3, add remaining MYOSD button bits, extend Java mapping so
   every Android keycode (M1/M2 etc.) is mappable. Fix right-stick axis classification
   in Java (the #104 bug — native RX/RY items already exist!).
2. 8 players: raise MYOSD_NUM_JOY 4→8 + Java-side device→player routing for >4 devices.
3. True pass-through (if still needed after 1+2): add a raw-gamepad JNI channel modeled
   on the existing setKeyData path, registering items dynamically per device.
4. Dual licensing note: MAME4droid says "dual-licensed" in headers — check exact terms
   when publishing the fork.

## Build Integration Progress (2026-07-05, "hit it")

Done on `rp6-android` branch:
1. **OSD overlay committed** — 31 `src/osd/myosd/` files + all 8 core patches applied
   (3 auto, 5 hand-applied due to 0.288 context drift; DAV HACK blocks preserved).
2. **Discovered upstream android scaffolding**: `make android-arm64` exists (GENie
   `--gcc=android-arm64 --targetos=android`, NDK clang toolchain) — but SDL-oriented.
3. **Wrote the DIY build glue seleuco doesn't ship**:
   - `scripts/src/osd/myosd.lua` — osd_myosd + ocore_myosd projects (modeled on sdl.lua;
     osdmodulesbuild() included → debugremote compiles in); links GLESv3/GLESv1_CM/EGL/
     OpenSLES/log/android/dl.
   - `scripts/src/osd/myosd_cfg.lua` — minimal defines.
   - `scripts/src/main.lua` — android+myosd → SharedLib named **libMAME4droid.so**
     (soname set), no SDL scaffolding/copies.
   - `makefile` — new **`make android-myosd`** target (needs only ANDROID_NDK_HOME,
     no SDL_INSTALL_ROOT).
4. **✅ BUILD SUCCEEDED (attempt 7, 2026-07-05)** — `make android-myosd SUBTARGET=tiny`
   produces **`libMAME4droid.so`** (117 MB, ELF arm64, Android 24+, NDK r28c, unstripped)
   with ALL `myosd_droid_*` JNI entry points exported (verified with llvm-nm).
   Error iteration log (each → fix → commit `d05525ae7d0`):
   - expat entropy → `BASE_TARGETOS = "unix"` in myosd.lua
   - machine.cpp/uiinput.cpp hunks misplaced at file scope by `patch` fuzz → relocated
   - ui.cpp escapes mangled to literal NUL/newlines by tooling → repaired via chr() script
   - bgfx draw module needs window.h → `removefiles` for myosd (no osdwindow layer)
   - myosd-droid.cpp reached for the app repo via `../../../` relative include →
     local copy of `com_seleuco_mame4droid_Emulator.h` in the OSD
   - netplay.cpp unused var vs -Werror → `[[maybe_unused]]`
   - link: unconditional `-lSDL2` + `-lqtdbg_<osd>` in main.lua → guarded off for myosd

Note for Phase 2 (WiFi debug): myosd's osd class stubs `init_debugger()`/
`wait_for_debugger()` as no-ops (myosd.h:74-76) — debugremote needs a small hook there.

## APK #1 SHIPPED (2026-07-05)

**`MAMEalicious-1.37-release.apk` (81 MB) built and pushed to the RP6's Downloads.**
Contains: tiny-subtarget core (L3/R3 native items enabled) + JNI shim + rebranded app.

Input changes included (the point of the fork):
- Native: `MYOSD_L3=1<<18 / MYOSD_R3=1<<19` defined; L3/R3 input items registered
  (input.cpp) → MAME sees 10 buttons + Select/Start per pad.
- Java: `L3_VALUE/R3_VALUE`; input set 16→18; **stick clicks freed** (were eaten as
  Start/Coin by defaults AND autodetect `mapTHUMBS`); factory defaults now
  Coin←BUTTON_SELECT, Start←BUTTON_START, L3←THUMBL, R3←THUMBR; define-keys UI lists
  the 2 new entries. M1/M2 (KEYCODE_BUTTON_C/Z) mappable via define-keys to any slot.
- Right-stick analog (AXIS_Z/RZ→RIGHT_STICK_DATA) already existed in Current upstream —
  verify on device whether #104 persists anyway.
- App commits live on the `mamealicious` branch, **pushed to the fork
  `github.com/tornotlukin/MAME4droid-Current`** (fork created 2026-07-05 via API).
  Remote safety: `origin` = our fork; `upstream` = seleuco (fetch-only — push URL
  set to an invalid sentinel so pushing upstream is impossible). Same neutering
  applied to `mamedev/mame` upstream in the mame repo.

Build notes: repo ships no gradle wrapper JAR → standalone Gradle 8.11.1 at
`H:\_DEV\android\gradle-8.11.1`; JAVA_HOME = Android Studio JBR; release uses debug
signing. APK: `android-MAME4droid/app/build/outputs/apk/release/`.

**Tiny core = few test games (no jrpacman/CPS yet)** — UI + input testing only.
Full-driver core build is the next step, then a full APK rebuild.

**FIRST-RUN BUG FIXED (2026-07-05):** app exited right after ROM-folder selection.
logcat: `dlopen failed: library "libc++_shared.so" not found` — the core links the NDK
shared C++ runtime and the APK didn't bundle it (upstream main.lua's android block copies
it; that step was stripped along with the SDL scaffolding). Fix: bundle
`$NDK/.../sysroot/usr/lib/aarch64-linux-android/libc++_shared.so` into
`jniLibs/arm64-v8a/` (now 3 .so in the APK). **Remember this for every core rebuild /
CI: libc++_shared.so must ship in the APK.**

## Directive: PURE MAME (2026-07-05)

User wants MAMEalicious to be **"MAME as it is on Windows"** — the native MAME UI
fullscreen, physical-controller input (RP6 built-ins + external pads), and NOTHING else
from MAME4droid unless an Android build genuinely requires it.

Step 1 (SHIPPED in current APK): pure-MAME **defaults** — touch overlay off in both
orientations, portrait fullscreen on. App already boots straight into MAME's own UI.

Step 2 (planned deep strip of the app layer):
- **REMOVE**: touch controller/virtual pad code paths (TouchController, TouchStick,
  ControlCustomizer, TiltSensor, TouchLightgun, TouchMouse), netplay UI/helper, scraper,
  web help, overlay/effects options, wizard cruft, Android-menu surfaces beyond the
  minimum.
- **KEEP (required for Android)**: Activity + GLES surface (renders MAME), JNI shim +
  Emulator bridge, native audio (OpenSL), **GameController/InputHandler physical-input
  path** (18-slot mapping incl. L3/R3), **SAF storage** (Android scoped storage — the
  only way to read ROM folders on Android 13), minimal settings (ROM path + define keys),
  app lifecycle/permissions.
- Approach: incremental removal with a working app at every step, on the `mamealicious`
  branch of the (to-be-forked) MAME4droid-Current repo.

Step 3 (config the Windows way): **manage settings via `mame.ini`** (standard MAME
config), not the app's SharedPreferences.

DECISIONS (2026-07-05):
- **Strip depth = DEFAULTS-ONLY** — keep MAME4droid code, default everything to pure-MAME
  (overlay off ✓, fullscreen on ✓). Reversible; options stay in Settings.
- **Config = mame.ini ONLY (Windows-style)** — neutralize the app's option-override layer
  so MAME's own mame.ini + TAB menu are authoritative.

CONFIRMED BY INSPECTION:
- Core uses `emulator_info::start_frontend(options, ...)` — the SAME desktop entry point
  that reads `mame.ini` from the working dir. `myosd_droid_initMyOSD` does `chdir(path)`,
  so mame.ini in that dir is already parsed natively. **mame.ini support already exists.**
- SAF file layer (`safOpenFile/ReadDir/...` callbacks in myosd-droid.cpp) transparently
  intercepts MAME's file ops → a rompath in mame.ini works, BUT Android 13 scoped storage
  still requires a one-time SAF *permission grant* for that tree (permission, not setting).
- What to neutralize: the app's `Emulator.setValue/setMyValue` pushes + any forced
  `-options` in the launch args that currently override mame.ini. TODO: enumerate every
  such call site in the Java layer and gate them off (or make them write mame.ini instead).

## APK #2 — THE FULL ONE (2026-07-05)

**`MAMEalicious-1.37-release.apk` (173 MB) with the FULL 42,880-driver core pushed to
the RP6.** Core: 553 MB unstripped → 415 MB after `llvm-strip --strip-unneeded`
(exports verified intact). Includes everything:
- All drivers (jrpacman 4P mods, CPS1/2/3, X-Men 6P...) — our fork IS the core
- L3/R3 + **M1/M2** native input items (named buttons in MAME's TAB menu)
- **"Retroid Pocket" autodetect profile** — device was previously BANNED (unknown names
  hit `banDev`); now fully mapped: M1/M2←BUTTON_C/Z, L3/R3←thumbs, Select=Coin,
  Start=Start, Mode=Option, L2/R2 digital→G/H
- Right-stick analog un-gated from isInGame (flows in frontend, desktop parity)
- 20-slot input set; libc++_shared bundled; pure-MAME defaults
- avgdvg.cpp reverted to stock (mangled vector-glow hack broke full build; re-apply later)

Expectation note: MAME menus don't navigate with M1/M2/right-stick (same as desktop);
verify via TAB → Input Settings → assign — they register by NAME (M1, M2, L3, R3).

## Proposed Plan (draft)

**Phase 0 — Reproduce & baseline**
- Clone `MAME4droid-Current` + `MAME4droid_Native`; build the stock APK in Android
  Studio; sideload on RP6.
- Log every KeyEvent/MotionEvent the app receives — ground truth of what Android
  delivers vs what MAME4droid drops (right stick, M1/M2, per-device IDs).
- Understand the native build: which MAME version, how the OSD is wired, where input
  crosses JNI.

**Phase 1 — Pass-through input (the core feature)**
- Replace the Java input funnel: register each Android input device as a distinct MAME
  input device with ALL buttons/axes (OSD input module work, Java→JNI→C++).
- Verify: TAB menu binds anything; right stick + M1/M2 visible; 2+ controllers appear
  as separate devices; fixes #104 by construction.

**Phase 2 — Our fork as the core**
- Point the native build at our mame fork (version-alignment decision here).
- jrpacman 4P on RP6 as the proving game; 8-player input routing test docked.
- Bonus experiment: compile debugremote into the Android core → WiFi debugging from PC.

**Phase 3 — Publish**
- RP6-tested fork on GitHub; offer the #104/pass-through work upstream as PRs.

## Next Steps

1. ~~Install NDK~~ ✅ DONE — 28.2.13676358 (r28c) verified at `H:\_DEV\android\SDK\ndk\`.
2. ~~Branch~~ ✅ DONE (2026-07-05) — full branch split executed: `llm-debugger` (pure
   debugger) + `jrpacman-4p` (pure game mods) + **`rp6-android`** (merge of both,
   pushed) as the composition branch.
3. Drop `MAME4droid-Current/src/` overlay into `rp6-android`; adapt build scripts
   (their howtobuild: modify makefile/lua to use the myosd OSD); build
   `libmame4droid-jni.so` with NDK; place in `app/src/main/jniLibs/arm64-v8a/`.
4. Build the APK with Gradle (`sdk.dir=H:\_DEV\android\SDK`), sideload to RP6 via adb.
5. Baseline input test on RP6 (what do right stick / M1/M2 emit), then start the input
   work: L3/R3 uncomment, keycode coverage, #104 axis fix, MYOSD_NUM_JOY→8.

---

## Resources & Links

- MAME4droid (Current): https://github.com/seleuco/MAME4droid-Current
- Native libs: https://github.com/seleuco/MAME4droid_Native
- Right-stick/button collision issue: https://github.com/seleuco/MAME4droid-Current/issues/104
- Virtual-controller redesign discussion: https://github.com/seleuco/MAME4droid-Current/issues/44
