// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria
/***************************************************************************

    Bally/Midway Jr. Pac-Man

    Games supported:
        * Jr. Pac-Man

    Known issues:
        * none

****************************************************************************

    Jr. Pac Man memory map (preliminary)

    0000-3fff ROM
    4000-47ff Video RAM (also color RAM)
    4800-4fff RAM
    8000-dfff ROM

    memory mapped ports:

    read:
    5000      P1
    5040      P2
    5080      DSW

    *
     * IN0 (all bits are inverted)
     * bit 7 : CREDIT
     * bit 6 : COIN 2
     * bit 5 : COIN 1
     * bit 4 : RACK TEST
     * bit 3 : DOWN player 1
     * bit 2 : RIGHT player 1
     * bit 1 : LEFT player 1
     * bit 0 : UP player 1
     *
    *
     * IN1 (all bits are inverted)
     * bit 7 : TABLE or UPRIGHT cabinet select (1 = UPRIGHT)
     * bit 6 : START 2
     * bit 5 : START 1
     * bit 4 : TEST SWITCH
     * bit 3 : DOWN player 2 (TABLE only)
     * bit 2 : RIGHT player 2 (TABLE only)
     * bit 1 : LEFT player 2 (TABLE only)
     * bit 0 : UP player 2 (TABLE only)
     *
    *
     * DSW1 (all bits are inverted)
     * bit 7 :  ?
     * bit 6 :  difficulty level
     *                       1 = Normal  0 = Harder
     * bit 5 :\ bonus pac at xx000 pts
     * bit 4 :/ 00 = 10000  01 = 15000  10 = 20000  11 = 30000
     * bit 3 :\ nr of lives
     * bit 2 :/ 00 = 1  01 = 2  10 = 3  11 = 5
     * bit 1 :\ play mode
     * bit 0 :/ 00 = free play   01 = 1 coin 1 credit
     *          10 = 1 coin 2 credits   11 = 2 coins 1 credit
     *

    write:
    4ff2-4ffd 6 pairs of two bytes:
              the first byte contains the sprite image number (bits 2-7), Y flip (bit 0),
              X flip (bit 1); the second byte the color
    5000      interrupt enable
    5001      sound enable
    5002      unused
    5003      flip screen
    5004      unused
    5005      unused
    5006      unused
    5007      coin counter
    5040-5044 sound voice 1 accumulator (nibbles) (used by the sound hardware only)
    5045      sound voice 1 waveform (nibble)
    5046-5049 sound voice 2 accumulator (nibbles) (used by the sound hardware only)
    504a      sound voice 2 waveform (nibble)
    504b-504e sound voice 3 accumulator (nibbles) (used by the sound hardware only)
    504f      sound voice 3 waveform (nibble)
    5050-5054 sound voice 1 frequency (nibbles)
    5055      sound voice 1 volume (nibble)
    5056-5059 sound voice 2 frequency (nibbles)
    505a      sound voice 2 volume (nibble)
    505b-505e sound voice 3 frequency (nibbles)
    505f      sound voice 3 volume (nibble)
    5062-506d Sprite coordinates, x/y pairs for 6 sprites
    5070      palette bank
    5071      colortable bank
    5073      background priority over sprites
    5074      char gfx bank
    5075      sprite gfx bank
    5080      scroll
    50c0      Watchdog reset

    I/O ports:
    OUT on port $0 sets the interrupt vector

***************************************************************************/

#include "emu.h"
#include "pacman.h"

#include "cpu/z80/z80.h"
#include "machine/74259.h"
#include "screen.h"
#include "speaker.h"


namespace {

class jrpacman_state : public pacman_state
{
public:
	jrpacman_state(const machine_config &mconfig, device_type type, const char *tag)
		: pacman_state(mconfig, type, tag)
	{ }

	void jrpacman(machine_config &config);

	void init_jrpacman();

private:
	void main_map(address_map &map) ATTR_COLD;
	void port_map(address_map &map) ATTR_COLD;
};



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

void jrpacman_state::main_map(address_map &map)
{
	map(0x0000, 0x3fff).rom();
	map(0x4000, 0x47ff).ram().w(FUNC(jrpacman_state::jrpacman_videoram_w)).share("videoram");
	map(0x4800, 0x4aff).ram();
	map(0x4b00, 0x4b07).ram().share("spritext");   // pac-man-4ever: 2 extended (software) sprites (slots 8,9)
	map(0x4b08, 0x4b11).ram().share("sprhi");      // pac-man-4ever: per-sprite high bank (code bit7) for 256-sprite set: [0..7]=hw sprites, [8..9]=extended
	map(0x4b12, 0x4fef).ram();
	map(0x4ff0, 0x4fff).ram().share("spriteram");
	map(0x5000, 0x503f).portr("P1");
	map(0x5000, 0x5007).w("latch1", FUNC(ls259_device::write_d0));
	map(0x5040, 0x507f).portr("P2");
	map(0x5040, 0x505f).w(m_namco_sound, FUNC(namco_wsg_device::pacman_sound_w));
	map(0x5060, 0x506f).writeonly().share("spriteram2");
	map(0x5070, 0x5077).w("latch2", FUNC(ls259_device::write_d0));
	map(0x5080, 0x50bf).portr("DSW1");
	map(0x5080, 0x5080).w(FUNC(jrpacman_state::jrpacman_scroll_w));
	map(0x50c0, 0x50c0).w(m_watchdog, FUNC(watchdog_timer_device::reset_w));
	// pac-man-4ever: extra simultaneous-player inputs (not on stock hardware)
	map(0x5100, 0x5100).portr("P3");
	map(0x5101, 0x5101).portr("P4");
	map(0x6000, 0x7fff).rom();     // pac-man-4ever: expansion ROM 2 (engine modules; plaintext - decrypt table is zero here)
	map(0x8000, 0xdfff).rom();
	map(0xe000, 0xffff).rom();     // pac-man-4ever: expansion ROM (screens/data; plaintext - decrypt table is zero here)
}


void jrpacman_state::port_map(address_map &map)
{
	map.global_mask(0xff);
	map(0, 0).w(FUNC(jrpacman_state::pacman_interrupt_vector_w));
}



/*************************************
 *
 *  Port definitions
 *
 *************************************/

static INPUT_PORTS_START( jrpacman )
	PORT_START("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_DIPNAME( 0x10, 0x10, "Rack Test (Cheat)" ) PORT_CODE(KEYCODE_F1)
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	// pac-man-4ever: P2 is now an independent simultaneous player (was COCKTAIL)
	PORT_START("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(2)
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	// pac-man-4ever: new simultaneous players 3 and 4 (read at 0x5100 / 0x5101)
	PORT_START("P3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0xd0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("P4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0xd0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("DSW1")
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Coinage ) )          PORT_DIPLOCATION("SW1:1,2")
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )            PORT_DIPLOCATION("SW1:3,4")   // pac-man-4ever: default 1 (per-round lives; ROM caps at 3)
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	// pac-man-4ever: Bonus Life repurposed (the award is patched out - extra lives have no
	// place in competitive rounds). Bit4 = IMMUNITY: ghosts can't kill players. A testing
	// switch - one human can drive several pacs through a full board clear.
	PORT_DIPNAME( 0x10, 0x00, "Immunity (Testing)" )        PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) )           PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Difficulty ) )       PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )          PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END



/*************************************
 *
 *  Graphics layouts
 *
 *************************************/

static const gfx_layout tilelayout =
{
	8,8,
	512,                 // pac-man-4ever: explicit count (was RGN_FRAC(1,2)) - decoupled from the now-larger gfx1 region
	2,
	{ 0, 4 },
	{ STEP4(8*8,1), STEP4(0*8,1) },
	{ STEP8(0*8,8) },
	16*8
};


static const gfx_layout spritelayout =
{
	16,16,
	256,                 // pac-man-4ever: L1 sprite expansion 128 -> 256 (was RGN_FRAC(1,2)); upper 128 = new-art headroom
	2,
	{ 0, 4 },
	{ STEP4(8*8,1), STEP4(16*8,1), STEP4(24*8,1), STEP4(0*8,1) },
	{ STEP8(0*8,8), STEP8(32*8,8) },
	64*8
};


static GFXDECODE_START( gfx_jrpacman )
	GFXDECODE_ENTRY( "gfx1", 0x0000, tilelayout,   0, 128 )
	GFXDECODE_ENTRY( "gfx1", 0x2000, spritelayout, 0, 128 )
GFXDECODE_END



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

void jrpacman_state::jrpacman(machine_config &config)
{
	pacman(config);

	// basic machine hardware
	// pac-man-4ever: the 4P board taps the crystal at /3 (6.144MHz, was /6): four pacs +
	// five ghosts exceed the stock Z80 budget (measured: main loop at ~55/120 with 4P).
	// Game speed is frame-locked (vblank IRQ), sound/video have their own clocks - the
	// CPU just stops missing frames.
	m_maincpu->set_clock(18.432_MHz_XTAL / 3);
	m_maincpu->set_addrmap(AS_PROGRAM, &jrpacman_state::main_map);
	m_maincpu->set_addrmap(AS_IO, &jrpacman_state::port_map);

	config.device_remove("mainlatch");

	ls259_device &latch1(LS259(config, "latch1")); // 5P
	latch1.q_out_cb<0>().set(FUNC(jrpacman_state::irq_mask_w));
	latch1.q_out_cb<1>().set("namco", FUNC(namco_wsg_device::sound_enable_w));
	latch1.q_out_cb<3>().set(FUNC(jrpacman_state::flipscreen_w));
	latch1.q_out_cb<7>().set(FUNC(jrpacman_state::coin_counter_w));

	ls259_device &latch2(LS259(config, "latch2")); // 1H
	latch2.q_out_cb<0>().set(FUNC(jrpacman_state::pengo_palettebank_w));
	latch2.q_out_cb<1>().set(FUNC(jrpacman_state::pengo_colortablebank_w));
	latch2.q_out_cb<3>().set(FUNC(jrpacman_state::jrpacman_bgpriority_w));
	latch2.q_out_cb<4>().set(FUNC(jrpacman_state::jrpacman_charbank_w));
	latch2.q_out_cb<5>().set(FUNC(jrpacman_state::jrpacman_spritebank_w));

	// video hardware
	m_gfxdecode->set_info(gfx_jrpacman);

	// pac-man-4ever widescreen: reveal the entire 36x54 tile maze (288x432) instead of
	// the stock 288x224 scrolling window. Bump the pixel clock proportionally so VBLANK
	// (and thus game speed) stays identical to stock: clock = XTAL/3 * 472/264.
	m_screen->set_raw(18.432_MHz_XTAL * 472 / 792, 384, 0, 288, 472, 0, 432);
	// Arcade monitors default to a 4:3 physical aspect, which (rotated 90) would squash our
	// now-wide playfield into a portrait window. Set square pixels so the window adopts the
	// true 432x288 wide proportion after ROT90.
	m_screen->set_physical_aspect(288, 432);

	MCFG_VIDEO_START_OVERRIDE(jrpacman_state,jrpacman)
}



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

/*

Jr. Pac-Man (11/9/83)

Label format:
+------------+
| JR.PAC-MAN |
|     8D     |
|   11/9/83  |
| @BALLY/MDWY|  <-- the "@" is actually the copyright "circled C"
+------------+

*/
ROM_START( jrpacman )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "jr.pac-man_8d_11-9-83.8d",    0x0000, 0x2000, CRC(e3fa972e) SHA1(5ea34621213c649ca2848ab31aab2cbe751723d4) )
	ROM_LOAD( "jr.pac-man_8e_11-9-83.8e",    0x2000, 0x2000, CRC(ec889e94) SHA1(8294e9e79f8fd19a419431fa690e6ac4a1302f58) )
	ROM_LOAD( "jr.pac-man_8h_11-9-83.8h",    0x8000, 0x2000, CRC(35f1fc6e) SHA1(b84b34560b9aae18b24274712b052283faa01730) )
	ROM_LOAD( "jr.pac-man_8j_11-9-83.8j",    0xa000, 0x2000, CRC(9737099e) SHA1(07d912a61824323c8fc1b8bd0da89172d4f70b91) )
	ROM_LOAD( "jr.pac-man_8k_11-9-83.8k",    0xc000, 0x2000, CRC(5252dd97) SHA1(18bd4d5381656120e4242811006c20776774de4d) )
	ROM_LOAD_OPTIONAL( "pac4eva.6x",         0x6000, 0x2000, CRC(d8f49994) SHA1(0631457264ff7f8d5fb1edc2c0211992a67c73e6) ) // pac-man-4ever: expansion ROM 2 (engine modules; plaintext)
	ROM_LOAD_OPTIONAL( "pac4eva.8x",         0xe000, 0x2000, CRC(d8f49994) SHA1(0631457264ff7f8d5fb1edc2c0211992a67c73e6) ) // pac-man-4ever: expansion ROM (plaintext; decrypt table is zero over 0xe000+)

	ROM_REGION( 0x6000, "gfx1", 0 )   // pac-man-4ever: L1 layout = tiles 0x2000 + sprites 0x4000 (256). Upper 128 sprites blank in the stock set, painted via the gfx import tool in modroms.
	ROM_LOAD( "jr.pac-man_2c_11-9-83.2c",    0x0000, 0x2000, CRC(0527ff9b) SHA1(37fe3176b0d125b7d629e108e7ebdc1196e4a132) ) /* tiles (512) */
	ROM_LOAD( "jr.pac-man_2e_11-9-83.2e",    0x2000, 0x4000, CRC(73477193) SHA1(f00a488958ea0438642d345693787bdf771219ad) ) /* sprites (256; stock file is 0x2000 -> upper half zero-filled) */

	ROM_REGION( 0x0120, "proms", 0 )
	ROM_LOAD_NIB_LOW ( "a290-27axv-bxhd.9e", 0x0000, 0x0100, CRC(029d35c4) SHA1(d9aa2dc442e9ac36cf3c346b9fb1aa745eaf3cb8) ) /* color palette (low bits) */
	ROM_LOAD_NIB_HIGH( "a290-27axv-cxhd.9f", 0x0000, 0x0100, CRC(eee34a79) SHA1(7561f8ccab2af85c111af6a02af6986eb67503e5) ) /* color palette (high bits) */
	ROM_LOAD( "a290-27axv-axhd.9p",          0x0020, 0x0100, CRC(9f6ea9d8) SHA1(62cf15513934d34641433c891a7f73bef82e2fb1) ) /* color lookup table */

	ROM_REGION( 0x0200, "namco", 0 )
	ROM_LOAD( "a290-27axv-dxhd.7p",          0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) ) /* waveform */
	ROM_LOAD( "a290-27axv-exhd.5s",          0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */
ROM_END

// pac4eva: "Pac-Man 4 EVA" (TORNOTLUKIN, 2026) — pac-man-4ever standalone set. Same board
// as jrpacman with the two expansion ROMs REQUIRED (not optional) and the L1 256-sprite gfx
// layout. Self-contained (parent 0) so it ships in the curated Modalicious build alone.
ROM_START( pac4eva )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "jr.pac-man_8d_11-9-83.8d",    0x0000, 0x2000, CRC(7799a7e6) SHA1(daa18744dd12743a5adc8cc43f780ae54cd14b3c) )
	ROM_LOAD( "jr.pac-man_8e_11-9-83.8e",    0x2000, 0x2000, CRC(40cdad13) SHA1(9c55443f7207f97aee24e55c1ca0646367ccbefc) )
	ROM_LOAD( "jr.pac-man_8h_11-9-83.8h",    0x8000, 0x2000, CRC(540a6039) SHA1(b061ca2ab893ebacdb67dd2646a8053be7e33373) )
	ROM_LOAD( "jr.pac-man_8j_11-9-83.8j",    0xa000, 0x2000, CRC(e788dfe2) SHA1(fa705b1ff20846e876b83fb7e1182182b4043759) )
	ROM_LOAD( "jr.pac-man_8k_11-9-83.8k",    0xc000, 0x2000, CRC(86516ff7) SHA1(df063ea4f23efdb7a311dece0068165aa077545f) )
	ROM_LOAD( "pac4eva.6x",                  0x6000, 0x2000, CRC(7e680231) SHA1(8551e0f4e0fe5faab00a122333d370e8d526729d) ) // expansion ROM 2 (engine modules; plaintext)
	ROM_LOAD( "pac4eva.8x",                  0xe000, 0x2000, CRC(adfbf49c) SHA1(b8315d3a743765b9f567c287dfb3f5ff23f2a85c) ) // expansion ROM (plaintext; decrypt table zero over 0xe000+)

	ROM_REGION( 0x6000, "gfx1", 0 )   // L1 layout = tiles 0x2000 + sprites 0x4000 (256)
	ROM_LOAD( "jr.pac-man_2c_11-9-83.2c",    0x0000, 0x2000, CRC(a624f5cb) SHA1(90809d9d30df183461c0c40f2da941a21fec6d5c) ) /* tiles (512) */
	ROM_LOAD( "jr.pac-man_2e_11-9-83.2e",    0x2000, 0x4000, CRC(a9d761f8) SHA1(20090c5a98db98e5cc768f3c886cfff864dfcb64) ) /* sprites (256) */

	ROM_REGION( 0x0120, "proms", 0 )
	ROM_LOAD_NIB_LOW ( "a290-27axv-bxhd.9e", 0x0000, 0x0100, CRC(029d35c4) SHA1(d9aa2dc442e9ac36cf3c346b9fb1aa745eaf3cb8) ) /* color palette (low bits) */
	ROM_LOAD_NIB_HIGH( "a290-27axv-cxhd.9f", 0x0000, 0x0100, CRC(eee34a79) SHA1(7561f8ccab2af85c111af6a02af6986eb67503e5) ) /* color palette (high bits) */
	ROM_LOAD( "a290-27axv-axhd.9p",          0x0020, 0x0100, CRC(2313697c) SHA1(e52560acd0d83ee8121c0b8c4981fc26e0f51b66) ) /* color lookup table */

	ROM_REGION( 0x0200, "namco", 0 )
	ROM_LOAD( "a290-27axv-dxhd.7p",          0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) ) /* waveform */
	ROM_LOAD( "a290-27axv-exhd.5s",          0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */
ROM_END

ROM_START( jrpacmanf )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "fast_jr.8d",                  0x0000, 0x2000, CRC(461e8b57) SHA1(42e25d384e653efb95a97bd64f55a8c3b3f71239) ) // only 1 byte difference
	ROM_LOAD( "jr.pac-man_8e_11-9-83.8e",    0x2000, 0x2000, CRC(ec889e94) SHA1(8294e9e79f8fd19a419431fa690e6ac4a1302f58) )
	ROM_LOAD( "jr.pac-man_8h_11-9-83.8h",    0x8000, 0x2000, CRC(35f1fc6e) SHA1(b84b34560b9aae18b24274712b052283faa01730) )
	ROM_LOAD( "jr.pac-man_8j_11-9-83.8j",    0xa000, 0x2000, CRC(9737099e) SHA1(07d912a61824323c8fc1b8bd0da89172d4f70b91) )
	ROM_LOAD( "jr.pac-man_8k_11-9-83.8k",    0xc000, 0x2000, CRC(5252dd97) SHA1(18bd4d5381656120e4242811006c20776774de4d) )

	ROM_REGION( 0x6000, "gfx1", 0 )   // pac-man-4ever: L1 layout = tiles 0x2000 + sprites 0x4000 (256). Upper 128 sprites blank in the stock set, painted via the gfx import tool in modroms.
	ROM_LOAD( "jr.pac-man_2c_11-9-83.2c",    0x0000, 0x2000, CRC(0527ff9b) SHA1(37fe3176b0d125b7d629e108e7ebdc1196e4a132) ) /* tiles (512) */
	ROM_LOAD( "jr.pac-man_2e_11-9-83.2e",    0x2000, 0x4000, CRC(73477193) SHA1(f00a488958ea0438642d345693787bdf771219ad) ) /* sprites (256; stock file is 0x2000 -> upper half zero-filled) */

	ROM_REGION( 0x0120, "proms", 0 )
	ROM_LOAD_NIB_LOW ( "a290-27axv-bxhd.9e", 0x0000, 0x0100, CRC(029d35c4) SHA1(d9aa2dc442e9ac36cf3c346b9fb1aa745eaf3cb8) ) /* color palette (low bits) */
	ROM_LOAD_NIB_HIGH( "a290-27axv-cxhd.9f", 0x0000, 0x0100, CRC(eee34a79) SHA1(7561f8ccab2af85c111af6a02af6986eb67503e5) ) /* color palette (high bits) */
	ROM_LOAD( "a290-27axv-axhd.9p",          0x0020, 0x0100, CRC(9f6ea9d8) SHA1(62cf15513934d34641433c891a7f73bef82e2fb1) ) /* color lookup table */

	ROM_REGION( 0x0200, "namco", 0 )
	ROM_LOAD( "a290-27axv-dxhd.7p",          0x0000, 0x0100, CRC(a9cc86bf) SHA1(bbcec0570aeceb582ff8238a4bc8546a23430081) ) /* waveform */
	ROM_LOAD( "a290-27axv-exhd.5s",          0x0100, 0x0100, CRC(77245b66) SHA1(0c4d0bee858b97632411c440bea6948a74759746) ) /* timing - not used */
ROM_END



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

void jrpacman_state::init_jrpacman()
{
	/* The encryption PALs garble bits 0, 2 and 7 of the ROMs. The encryption */
	/* scheme is complex (basically it's a state machine) and can only be */
	/* faithfully emulated at run time. To avoid the performance hit that would */
	/* cause, here we have a table of the values which must be XORed with */
	/* each memory region to obtain the decrypted bytes. */
	/* Decryption table provided by David Caldwell (david@indigita.com) */
	/* For an accurate reproduction of the encryption, see jrcrypt.c */
	static const struct {
		int count;
		int value;
	} table[] =
	{
		{ 0x00C1, 0x00 },{ 0x0002, 0x80 },{ 0x0004, 0x00 },{ 0x0006, 0x80 },
		{ 0x0003, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0004, 0x80 },
		{ 0x9968, 0x00 },{ 0x0001, 0x80 },{ 0x0002, 0x00 },{ 0x0001, 0x80 },
		{ 0x0009, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0001, 0x80 },
		{ 0x00AF, 0x00 },{ 0x000E, 0x04 },{ 0x0002, 0x00 },{ 0x0004, 0x04 },
		{ 0x001E, 0x00 },{ 0x0001, 0x80 },{ 0x0002, 0x00 },{ 0x0001, 0x80 },
		{ 0x0002, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0002, 0x80 },
		{ 0x0009, 0x00 },{ 0x0002, 0x80 },{ 0x0083, 0x00 },{ 0x0001, 0x04 },
		{ 0x0001, 0x01 },{ 0x0001, 0x00 },{ 0x0002, 0x05 },{ 0x0001, 0x00 },
		{ 0x0003, 0x04 },{ 0x0003, 0x01 },{ 0x0002, 0x00 },{ 0x0001, 0x04 },
		{ 0x0003, 0x01 },{ 0x0003, 0x00 },{ 0x0003, 0x04 },{ 0x0001, 0x01 },
		{ 0x002E, 0x00 },{ 0x0078, 0x01 },{ 0x0001, 0x04 },{ 0x0001, 0x05 },
		{ 0x0001, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },
		{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },{ 0x0001, 0x01 },
		{ 0x0001, 0x04 },{ 0x0002, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },
		{ 0x0001, 0x05 },{ 0x0001, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },
		{ 0x0002, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },
		{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0001, 0x05 },{ 0x0001, 0x00 },
		{ 0x01B0, 0x01 },{ 0x0001, 0x00 },{ 0x0002, 0x01 },{ 0x00AD, 0x00 },
		{ 0x0031, 0x01 },{ 0x005C, 0x00 },{ 0x0005, 0x01 },{ 0x604E, 0x00 },
		{ 0,0 }
	};

	uint8_t *RAM = memregion("maincpu")->base();
	for (int i = 0, A = 0; table[i].count; i++)
		for (int j = 0; j < table[i].count; j++)
			RAM[A++] ^= table[i].value;
}

} // anonymous namespace


/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1983, jrpacman,  0,        jrpacman, jrpacman, jrpacman_state, init_jrpacman, ROT90, "Bally Midway", "Jr. Pac-Man (11/9/83)",      MACHINE_SUPPORTS_SAVE )
GAME( 1983, jrpacmanf, jrpacman, jrpacman, jrpacman, jrpacman_state, init_jrpacman, ROT90, "hack",         "Jr. Pac-Man (speedup hack)", MACHINE_SUPPORTS_SAVE )
GAME( 2026, pac4eva,   0,        jrpacman, jrpacman, jrpacman_state, init_jrpacman, ROT90, "TORNOTLUKIN",  "Pac-Man 4 EVA",              MACHINE_SUPPORTS_SAVE )
