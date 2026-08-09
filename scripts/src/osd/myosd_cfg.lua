-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

-- Configuration shared by all projects when building the myosd (MAME4droid
-- Android) OSD. Kept deliberately minimal: the myosd sources are
-- self-contained and the Android toolchain provides __ANDROID__.

dofile('modules.lua')

defines {
	"OSD_MYOSD",
	"OSD_DROID",
	"SDLMAME_NOASM=1",
	"USE_QTDEBUG=0",
	"USE_OPENGL=0",
}
