-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   myosd.lua
--
--   Rules for the building of the MAME4droid (myosd) Android OSD
--   Produces libMAME4droid.so consumed by the MAME4droid app's JNI shim.
--
---------------------------------------------------------------------------

dofile("modules.lua")


function maintargetosdoptions(_target,_subtarget)
	osdmodulestargetconf()

	-- Android system libraries used by the myosd OSD
	links {
		"GLESv3",
		"GLESv1_CM",
		"EGL",
		"OpenSLES",
		"log",
		"android",
		"dl",
	}
end


project ("osd_" .. _OPTIONS["osd"])
	uuid (os.uuid("osd_" .. _OPTIONS["osd"]))
	kind (LIBTYPE)

	dofile("myosd_cfg.lua")
	osdmodulesbuild()

	includedirs {
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/devices", -- accessing imagedev from debugger
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "src/osd/modules/file",
		MAME_DIR .. "src/osd/modules/render",
		MAME_DIR .. "3rdparty",
		MAME_DIR .. "src/frontend/mame", -- ui.cpp DAV hooks reach into frontend
		MAME_DIR .. "src/osd/myosd",
	}

	files {
		MAME_DIR .. "src/osd/osdepend.h",
		MAME_DIR .. "src/osd/myosd/myosd.h",
		MAME_DIR .. "src/osd/myosd/myosd_core.h",
		MAME_DIR .. "src/osd/myosd/myosd_saf.h",
		MAME_DIR .. "src/osd/myosd/myosdmain.cpp",
		MAME_DIR .. "src/osd/myosd/myosd-droid.cpp",
		MAME_DIR .. "src/osd/myosd/myosd-droid.h",
		MAME_DIR .. "src/osd/myosd/input.cpp",
		MAME_DIR .. "src/osd/myosd/video.cpp",
		MAME_DIR .. "src/osd/myosd/sound.cpp",
		MAME_DIR .. "src/osd/myosd/opensl_snd.cpp",
		MAME_DIR .. "src/osd/myosd/opensl_snd.h",
		MAME_DIR .. "src/osd/myosd/netplay.cpp",
		MAME_DIR .. "src/osd/myosd/netplay.h",
		MAME_DIR .. "src/osd/myosd/skt_netplay.cpp",
		MAME_DIR .. "src/osd/myosd/skt_netplay.h",
		MAME_DIR .. "src/osd/myosd/renderer/myosd_renderer.h",
		MAME_DIR .. "src/osd/myosd/renderer/gles1_renderer.cpp",
		MAME_DIR .. "src/osd/myosd/renderer/gles1_renderer.h",
		MAME_DIR .. "src/osd/myosd/renderer/gles3_renderer.cpp",
		MAME_DIR .. "src/osd/myosd/renderer/gles3_renderer.h",
		MAME_DIR .. "src/osd/myosd/renderer/filter_shader.cpp",
		MAME_DIR .. "src/osd/myosd/renderer/filter_shader.h",
	}


project ("ocore_" .. _OPTIONS["osd"])
	uuid (os.uuid("ocore_" .. _OPTIONS["osd"]))
	kind (LIBTYPE)

	removeflags {
		"SingleOutputDir",
	}

	dofile("myosd_cfg.lua")

	includedirs {
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "src/osd/myosd",
	}

	files {
		MAME_DIR .. "src/osd/osdcore.cpp",
		MAME_DIR .. "src/osd/osdcore.h",
		MAME_DIR .. "src/osd/osdfile.h",
		MAME_DIR .. "src/osd/strconv.cpp",
		MAME_DIR .. "src/osd/strconv.h",
		MAME_DIR .. "src/osd/osdsync.cpp",
		MAME_DIR .. "src/osd/osdsync.h",
		MAME_DIR .. "src/osd/modules/osdmodule.cpp",
		MAME_DIR .. "src/osd/modules/osdmodule.h",
		MAME_DIR .. "src/osd/myosd/osdlib.cpp",
		MAME_DIR .. "src/osd/modules/lib/osdlib.h",
		-- myosd ships its own posix file layer (SAF-aware)
		MAME_DIR .. "src/osd/myosd/file/posixdir.cpp",
		MAME_DIR .. "src/osd/myosd/file/posixfile.cpp",
		MAME_DIR .. "src/osd/myosd/file/posixfile.h",
		MAME_DIR .. "src/osd/myosd/file/posixptty.cpp",
		MAME_DIR .. "src/osd/myosd/file/posixsocket.cpp",
	}
