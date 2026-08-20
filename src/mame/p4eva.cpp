// license:BSD-3-Clause
// copyright-holders:TORNOTLUKIN
/***************************************************************************

    p4eva.cpp

    Per-target branding constants for the standalone Pac-Man 4 EVA build.

    MAME picks src/mame/<subtarget>.cpp over src/mame/mame.cpp when the file
    exists (scripts/src/main.lua), so this is the supported way to rename the
    application without patching anything shared. The MAME copyright line is
    kept intact below -- this build is MAME, and says so.

***************************************************************************/

#include "emu.h"
#include "main.h"

#define APPNAME                 "Pac-Man 4 EVA"
#define APPNAME_LOWER           "pac4eva"
#define CONFIGNAME              "pac4eva"
#define COPYRIGHT               "Pac-Man 4 EVA (c) 2026 TORNOTLUKIN\nBuilt on MAME - Copyright MAMEdev and contributors\nhttps://mamedev.org"
#define COPYRIGHT_INFO          "Pac-Man 4 EVA (c) 2026 TORNOTLUKIN - built on MAME, Copyright MAMEdev and contributors"

const char * emulator_info::get_appname() { return APPNAME;}
const char * emulator_info::get_appname_lower() { return APPNAME_LOWER;}
const char * emulator_info::get_configname() { return CONFIGNAME;}
const char * emulator_info::get_copyright() { return COPYRIGHT;}
const char * emulator_info::get_copyright_info() { return COPYRIGHT_INFO;}
