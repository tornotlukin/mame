CPUS["DSP16"] = true
CPUS["M680X0"] = true
CPUS["S2650"] = true
CPUS["Z80"] = true
MACHINES["TTL74157"] = true
MACHINES["TTL74259"] = true
MACHINES["EEPROMDEV"] = true
MACHINES["EEPROMDEV"] = true
MACHINES["GEN_LATCH"] = true
MACHINES["TIMEKPR"] = true
MACHINES["UPD4701"] = true
MACHINES["WATCHDOG"] = true
MACHINES["Z80DAISY"] = true
SOUNDS["AY8910"] = true
SOUNDS["NAMCO"] = true
SOUNDS["OKIADPCM"] = true
SOUNDS["OKIM6295"] = true
SOUNDS["QSOUND"] = true
SOUNDS["SN76496"] = true
SOUNDS["YM2151"] = true

function createProjects_mame_modalicious(_target, _subtarget)
    project ("mame_modalicious")
    targetsubdir(_target .."_" .. _subtarget)
    kind (LIBTYPE)
    uuid (os.uuid("drv-mame-modalicious"))
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
        MAME_DIR .. "src/mame/capcom/cps1.cpp",
        MAME_DIR .. "src/mame/capcom/cps1.h",
        MAME_DIR .. "src/mame/capcom/cps1_v.cpp",
        MAME_DIR .. "src/mame/capcom/cps2.cpp",
        MAME_DIR .. "src/mame/capcom/cps2comm.cpp",
        MAME_DIR .. "src/mame/capcom/cps2comm.h",
        MAME_DIR .. "src/mame/capcom/cps2crypt.cpp",
        MAME_DIR .. "src/mame/capcom/cps2crypt.h",
        MAME_DIR .. "src/mame/capcom/kabuki.cpp",
        MAME_DIR .. "src/mame/capcom/kabuki.h",
        MAME_DIR .. "src/mame/pacman/jumpshot.cpp",
        MAME_DIR .. "src/mame/pacman/jumpshot.h",
        MAME_DIR .. "src/mame/pacman/pac4eva.cpp",
        MAME_DIR .. "src/mame/pacman/pacman.cpp",
        MAME_DIR .. "src/mame/pacman/pacman.h",
        MAME_DIR .. "src/mame/pacman/pacman_m.cpp",
        MAME_DIR .. "src/mame/pacman/pacman_v.cpp",
        MAME_DIR .. "src/mame/pacman/pacplus.cpp",
        MAME_DIR .. "src/mame/pacman/pacplus.h",
    }
end

function linkProjects_mame_modalicious(_target, _subtarget)
    links {
        "mame_modalicious",
    }
end
