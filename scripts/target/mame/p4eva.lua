-- license:BSD-3-Clause
-- copyright-holders:TORNOTLUKIN

---------------------------------------------------------------------------
--
--   p4eva.lua
--
--   Standalone build target for Pac-Man 4 EVA.
--   Use:  make SUBTARGET=p4eva
--
--   This is the PRESENTATION build: one game, one exe, no library.
--   The subtarget is named "p4eva" and NOT "pac4eva" on purpose. MAME keys
--   two things to the subtarget name -- the branding file src/mame/<sub>.cpp
--   and the driver list src/mame/<sub>.lst -- so naming it "pac4eva" would
--   quietly rebrand and re-filter the DEVELOPMENT exe that tools/build_mame.sh
--   produces, which still needs jrpacman in its driver list for the debugger
--   disassembly flow. Different name, zero interference.
--
--   pacman.cpp is compiled even though only pac4eva is shipped: irq_mask_w,
--   coin_counter_w and pacman_interrupt_vector_w are defined there and
--   pac4eva_state inherits them. p4eva.lst is what trims the driver list down
--   to the single title -- moving those three functions out of a shared file
--   would diverge this branch from upstream for no gain.
--
---------------------------------------------------------------------------


--------------------------------------------------
-- CPU cores.  Z80 runs the game; S2650 is pulled
-- in by the bootleg sets inside pacman.cpp.
--------------------------------------------------

CPUS["Z80"] = true
CPUS["S2650"] = true

-- Required by the Z80 CORE itself, not by any pacman driver: z80_device derives
-- from z80_daisy_chain_interface, so omitting this fails at LINK time with 17
-- undefined references and no hint about the cause. Grepping the drivers'
-- #includes will never reveal it.
MACHINES["Z80DAISY"] = true

--------------------------------------------------
-- Sound cores.  NAMCO is the WSG the game plays
-- through; AY8910/SN76496 are pacman.cpp bootlegs.
--------------------------------------------------

SOUNDS["NAMCO"] = true
SOUNDS["AY8910"] = true
SOUNDS["SN76496"] = true

--------------------------------------------------
-- Machine cores.  Exactly the four headers the
-- pacman sources include (nvram is unconditional).
--------------------------------------------------

MACHINES["TTL74259"] = true
MACHINES["GEN_LATCH"] = true
MACHINES["WATCHDOG"] = true


function createProjects_mame_p4eva(_target, _subtarget)
	project ("mame_p4eva")
	targetsubdir(_target .."_" .. _subtarget)
	kind (LIBTYPE)
	uuid (os.uuid("drv-mame-p4eva"))
	addprojectflags()

	includedirs {
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/devices",
		MAME_DIR .. "src/mame/shared",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "src/lib/netlist",
		MAME_DIR .. "3rdparty",
		GEN_DIR  .. "mame/layout",
		ext_includedir("asio"),
		ext_includedir("flac"),
		ext_includedir("glm"),
		ext_includedir("jpeg"),
		ext_includedir("rapidjson"),
		ext_includedir("zlib"),
	}

	files{
		MAME_DIR .. "src/mame/pacman/pac4eva.cpp",
		MAME_DIR .. "src/mame/pacman/pacman.cpp",
		MAME_DIR .. "src/mame/pacman/pacman.h",
		MAME_DIR .. "src/mame/pacman/pacman_m.cpp",
		MAME_DIR .. "src/mame/pacman/pacman_v.cpp",
		MAME_DIR .. "src/mame/pacman/jumpshot.cpp",
		MAME_DIR .. "src/mame/pacman/jumpshot.h",
		MAME_DIR .. "src/mame/pacman/pacplus.cpp",
		MAME_DIR .. "src/mame/pacman/pacplus.h",
	}
end

function linkProjects_mame_p4eva(_target, _subtarget)
	links {
		"mame_p4eva",
	}
end
