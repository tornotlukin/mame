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

-- Android is unix-like; drives 3rdparty configs (expat entropy, etc.)
-- and unix conditionals throughout the build scripts.
BASE_TARGETOS = "unix"


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

	-- OSD_DROID registers only the Android modules (see upstream's osdobj_common
	-- patch), so the SDL-coupled bgfx renderer is neither needed nor buildable here.
	removefiles {
		MAME_DIR .. "src/osd/modules/render/drawbgfx.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfxutil.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/*.cpp",
	}


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
		MAME_DIR .. "src/osd/myosd/netplay",
		MAME_DIR .. "src/osd/myosd/droid",
		MAME_DIR .. "src/osd/myosd/modules",
		MAME_DIR .. "src/osd/myosd/renderer",
	}

	files {
		MAME_DIR .. "src/osd/osdepend.h",
		MAME_DIR .. "src/osd/modules/osdwindow.cpp",
		MAME_DIR .. "src/osd/modules/osdwindow.h",
		MAME_DIR .. "src/osd/myosd/input.cpp",
		MAME_DIR .. "src/osd/myosd/myosd.h",
		MAME_DIR .. "src/osd/myosd/myosd_core.h",
		MAME_DIR .. "src/osd/myosd/myosdmain.cpp",
		MAME_DIR .. "src/osd/myosd/myosdopts.cpp",
		MAME_DIR .. "src/osd/myosd/myosdopts.h",
		MAME_DIR .. "src/osd/myosd/video.cpp",
		MAME_DIR .. "src/osd/myosd/window.cpp",
		MAME_DIR .. "src/osd/myosd/window.h",
		MAME_DIR .. "src/osd/myosd/droid/droid_font.cpp",
		MAME_DIR .. "src/osd/myosd/droid/myosd_droid.cpp",
		MAME_DIR .. "src/osd/myosd/droid/myosd_droid.h",
		MAME_DIR .. "src/osd/myosd/droid/com_seleuco_mame4droid_Emulator.h",
		MAME_DIR .. "src/osd/myosd/droid/myosd_platform.h",
		MAME_DIR .. "src/osd/myosd/droid/myosd_saf.h",
		MAME_DIR .. "src/osd/myosd/droid/opensl_snd.cpp",
		MAME_DIR .. "src/osd/myosd/droid/opensl_snd.h",
		MAME_DIR .. "src/osd/myosd/modules/drawmyosd.cpp",
		MAME_DIR .. "src/osd/myosd/modules/font_myosd.cpp",
		MAME_DIR .. "src/osd/myosd/modules/input_myosd.cpp",
		MAME_DIR .. "src/osd/myosd/modules/monitor_myosd.cpp",
		MAME_DIR .. "src/osd/myosd/modules/myosd_sound.cpp",
		MAME_DIR .. "src/osd/myosd/netplay/myosd_netplay.cpp",
		MAME_DIR .. "src/osd/myosd/netplay/myosd_netplay.h",
		MAME_DIR .. "src/osd/myosd/netplay/netplay.cpp",
		MAME_DIR .. "src/osd/myosd/netplay/netplay.h",
		MAME_DIR .. "src/osd/myosd/netplay/skt_netplay.cpp",
		MAME_DIR .. "src/osd/myosd/netplay/skt_netplay.h",
		MAME_DIR .. "src/osd/myosd/renderer/filter_shader.cpp",
		MAME_DIR .. "src/osd/myosd/renderer/filter_shader.h",
		MAME_DIR .. "src/osd/myosd/renderer/gl_utils.hxx",
		MAME_DIR .. "src/osd/myosd/renderer/gles1_renderer.cpp",
		MAME_DIR .. "src/osd/myosd/renderer/gles1_renderer.h",
		MAME_DIR .. "src/osd/myosd/renderer/gles3_renderer.cpp",
		MAME_DIR .. "src/osd/myosd/renderer/gles3_renderer.h",
		MAME_DIR .. "src/osd/myosd/renderer/myosd_renderer.h",
		MAME_DIR .. "src/osd/myosd/renderer/shader_sources.hxx",
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
		MAME_DIR .. "src/osd/myosd/netplay",
		MAME_DIR .. "src/osd/myosd/droid",
		MAME_DIR .. "src/osd/myosd/modules",
		MAME_DIR .. "src/osd/myosd/renderer",
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
		MAME_DIR .. "src/osd/modules/lib/osdlib.h",
		-- myosd ships its own posix file layer (SAF-aware)
		MAME_DIR .. "src/osd/myosd/droid/osdlib.cpp",
		MAME_DIR .. "src/osd/myosd/file/posixdir.cpp",
		MAME_DIR .. "src/osd/myosd/file/posixfile.cpp",
		MAME_DIR .. "src/osd/myosd/file/posixfile.h",
		MAME_DIR .. "src/osd/myosd/file/posixptty.cpp",
		MAME_DIR .. "src/osd/myosd/file/posixsocket.cpp",
	}
