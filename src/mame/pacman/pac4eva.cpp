// license:BSD-3-Clause
// copyright-holders:TORNOTLUKIN
/***************************************************************************

    Pac-Man 4 EVA  (pac4eva)

    A 4-player competitive widescreen conversion of Bally/Midway's Jr. Pac-Man,
    built as a program-ROM patch set plus two plaintext expansion ROMs. This
    driver stands in for the daughterboard mods a conversion kit would have
    carried; the game logic itself lives entirely in the (encrypted) program ROM.

    Board mods vs a stock Jr. Pac-Man:
      - Z80 clocked at 18.432MHz/3 (6.144MHz, stock is /6). Four pacs and five
        ghosts exceed the stock CPU budget; pacing is vblank-locked and sound
        has its own clock, so game speed and pitch are unchanged - the CPU just
        stops missing frames.
      - The full 36x54 tile maze (288x432) is displayed at once instead of the
        stock 288x224 scrolling window. The pixel clock is scaled so VBLANK -
        and therefore game speed - matches stock exactly.
      - Sprites: 8 hardware + 2 software (RAM at 0x4b00), and the shape set is
        expanded 128 -> 256 via a per-sprite high-bank RAM at 0x4b08.
      - Two expansion ROMs at 0x6000-0x7fff and 0xe000-0xffff. Both are
        PLAINTEXT: the Jr. Pac-Man decryption table is all zero over those
        ranges, so no extra decode logic is needed.
      - Four independent 4-way joysticks. P2 is promoted out of cocktail mode
        and P3/P4 are new ports at 0x5100/0x5101.
      - DIPs repurposed for competitive play: per-round Lives, an Immunity
        testing switch, and a 4-way Difficulty scale (see below).

    The program ROM is encrypted exactly as stock Jr. Pac-Man (the PALs garble
    bits 0, 2 and 7), so init_pac4eva applies the same XOR table.

    Project: https://github.com/tornotlukin/pacman-4eva

***************************************************************************/

#include "emu.h"
#include "pacman.h"

#include "cpu/z80/z80.h"
#include "machine/74259.h"
#include "screen.h"
#include "speaker.h"


namespace {

class pac4eva_state : public pacman_state
{
public:
	pac4eva_state(const machine_config &mconfig, device_type type, const char *tag)
		: pacman_state(mconfig, type, tag)
	{ }

	void pac4eva(machine_config &config);

	void init_pac4eva();

private:
	void main_map(address_map &map) ATTR_COLD;
	void port_map(address_map &map) ATTR_COLD;
};



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

void pac4eva_state::main_map(address_map &map)
{
	map(0x0000, 0x3fff).rom();
	map(0x4000, 0x47ff).ram().w(FUNC(pac4eva_state::jrpacman_videoram_w)).share("videoram");
	map(0x4800, 0x4aff).ram();
	map(0x4b00, 0x4b07).ram().share("spritext");   // 2 extended (software) sprites (slots 8,9)
	map(0x4b08, 0x4b11).ram().share("sprhi");      // per-sprite high bank (code bit7) for the 256-shape set: [0..7]=hw sprites, [8..9]=extended
	map(0x4b12, 0x4fef).ram();
	map(0x4ff0, 0x4fff).ram().share("spriteram");
	map(0x5000, 0x503f).portr("P1");
	map(0x5000, 0x5007).w("latch1", FUNC(ls259_device::write_d0));
	map(0x5040, 0x507f).portr("P2");
	map(0x5040, 0x505f).w(m_namco_sound, FUNC(namco_wsg_device::pacman_sound_w));
	map(0x5060, 0x506f).writeonly().share("spriteram2");
	map(0x5070, 0x5077).w("latch2", FUNC(ls259_device::write_d0));
	map(0x5080, 0x50bf).portr("DSW1");
	map(0x5080, 0x5080).w(FUNC(pac4eva_state::jrpacman_scroll_w));
	map(0x50c0, 0x50c0).w(m_watchdog, FUNC(watchdog_timer_device::reset_w));
	map(0x5100, 0x5100).portr("P3");               // extra simultaneous-player inputs
	map(0x5101, 0x5101).portr("P4");
	map(0x6000, 0x7fff).rom();                     // expansion ROM 2 (engine modules + maze tables; plaintext)
	map(0x8000, 0xdfff).rom();
	map(0xe000, 0xffff).rom();                     // expansion ROM (screens, ghost engine; plaintext)
}


void pac4eva_state::port_map(address_map &map)
{
	map.global_mask(0xff);
	map(0, 0).w(FUNC(pac4eva_state::pacman_interrupt_vector_w));
}



/*************************************
 *
 *  Port definitions
 *
 *************************************/

static INPUT_PORTS_START( pac4eva )
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

	// P2 is an independent simultaneous player (stock Jr. Pac-Man muxed it as COCKTAIL)
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

	// simultaneous players 3 and 4 (read at 0x5100 / 0x5101 - not on stock hardware)
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
	// Lives are a PER-ROUND survival budget, reset every round (default 1).
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )            PORT_DIPLOCATION("SW1:3,4")
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	// Stock Bonus Life is repurposed - the award is patched out of the ROM, since extra
	// lives have no place in competitive rounds. Immunity is a testing switch: ghosts
	// cannot kill players, so one person can drive several pacs through a full board.
	PORT_DIPNAME( 0x10, 0x00, "Immunity (Testing)" )        PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	// 4-way difficulty. Ramp walks the authored difficulty ramp one entry every N rounds;
	// the three fixed settings PIN one ramp entry for the whole game. The ROM reads this
	// same 0x60 field in tools/asm/tourdiff.asm - KEEP THE TWO IN SYNC. Default must stay
	// 0x00 (Ramp): a default-on bit here is read as a difficulty tier at boot.
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Difficulty ) )       PORT_DIPLOCATION("SW1:6,7")
	PORT_DIPSETTING(    0x00, "Ramp (progressive)" )
	PORT_DIPSETTING(    0x20, "Easy (fixed)" )
	PORT_DIPSETTING(    0x40, "Medium (fixed)" )
	PORT_DIPSETTING(    0x60, "Hard (fixed)" )
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
	512,                 // explicit count (stock uses RGN_FRAC(1,2)) - decoupled from the larger gfx1 region
	2,
	{ 0, 4 },
	{ STEP4(8*8,1), STEP4(0*8,1) },
	{ STEP8(0*8,8) },
	16*8
};


static const gfx_layout spritelayout =
{
	16,16,
	256,                 // sprite shape set expanded 128 -> 256; upper 128 = new art
	2,
	{ 0, 4 },
	{ STEP4(8*8,1), STEP4(16*8,1), STEP4(24*8,1), STEP4(0*8,1) },
	{ STEP8(0*8,8), STEP8(32*8,8) },
	64*8
};


static GFXDECODE_START( gfx_pac4eva )
	GFXDECODE_ENTRY( "gfx1", 0x0000, tilelayout,   0, 128 )
	GFXDECODE_ENTRY( "gfx1", 0x2000, spritelayout, 0, 128 )
GFXDECODE_END



/*************************************
 *
 *  Machine driver
 *
 *************************************/

void pac4eva_state::pac4eva(machine_config &config)
{
	pacman(config);

	// basic machine hardware
	// The 4P board taps the crystal at /3 (6.144MHz, stock /6): four pacs and five ghosts
	// exceed the stock Z80 budget (measured: main loop at ~55/120 laps with a full table).
	// Game speed is frame-locked (vblank IRQ) and sound clocks separately, so speed and
	// pitch are unchanged - the CPU just stops missing frames.
	m_maincpu->set_clock(18.432_MHz_XTAL / 3);
	m_maincpu->set_addrmap(AS_PROGRAM, &pac4eva_state::main_map);
	m_maincpu->set_addrmap(AS_IO, &pac4eva_state::port_map);

	config.device_remove("mainlatch");

	ls259_device &latch1(LS259(config, "latch1")); // 5P
	latch1.q_out_cb<0>().set(FUNC(pac4eva_state::irq_mask_w));
	latch1.q_out_cb<1>().set("namco", FUNC(namco_wsg_device::sound_enable_w));
	latch1.q_out_cb<3>().set(FUNC(pac4eva_state::flipscreen_w));
	latch1.q_out_cb<7>().set(FUNC(pac4eva_state::coin_counter_w));

	ls259_device &latch2(LS259(config, "latch2")); // 1H
	latch2.q_out_cb<0>().set(FUNC(pac4eva_state::pengo_palettebank_w));
	latch2.q_out_cb<1>().set(FUNC(pac4eva_state::pengo_colortablebank_w));
	latch2.q_out_cb<3>().set(FUNC(pac4eva_state::jrpacman_bgpriority_w));
	latch2.q_out_cb<4>().set(FUNC(pac4eva_state::jrpacman_charbank_w));
	latch2.q_out_cb<5>().set(FUNC(pac4eva_state::jrpacman_spritebank_w));

	// video hardware
	m_gfxdecode->set_info(gfx_pac4eva);

	// Widescreen: reveal the entire 36x54 tile maze (288x432) instead of the stock 288x224
	// scrolling window. The pixel clock is bumped proportionally so VBLANK - and therefore
	// game speed - stays identical to stock: clock = XTAL/3 * 472/264.
	m_screen->set_raw(18.432_MHz_XTAL * 472 / 792, 384, 0, 288, 472, 0, 432);
	// Arcade monitors default to a 4:3 physical aspect, which (rotated 90) would squash the
	// now-wide playfield into a portrait window. Square pixels make the window adopt the
	// true 432x288 wide proportion after ROT90.
	m_screen->set_physical_aspect(288, 432);

	MCFG_VIDEO_START_OVERRIDE(pac4eva_state,jrpacman)
}



/*************************************
 *
 *  ROM definition
 *
 *************************************/

/*
    The romset is produced by the project's build pipeline (tools/build.sh ->
    build/modroms/pac4eva.zip). The five program ROMs keep their Jr. Pac-Man
    filenames because they occupy the same board positions; their contents are
    the patched, re-encrypted program. pac4eva.6x / .8x are the expansion ROMs.

    NOTE: these hashes describe a specific build. Content changes whenever the
    asm, artwork, mazes or colour schemes change, so refresh them from the built
    set when cutting a release (a mismatch is only a warning - the set still runs).
*/
ROM_START( pac4eva )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "jr.pac-man_8d_11-9-83.8d",    0x0000, 0x2000, CRC(7799a7e6) SHA1(daa18744dd12743a5adc8cc43f780ae54cd14b3c) )
	ROM_LOAD( "jr.pac-man_8e_11-9-83.8e",    0x2000, 0x2000, CRC(40cdad13) SHA1(9c55443f7207f97aee24e55c1ca0646367ccbefc) )
	ROM_LOAD( "jr.pac-man_8h_11-9-83.8h",    0x8000, 0x2000, CRC(540a6039) SHA1(b061ca2ab893ebacdb67dd2646a8053be7e33373) )
	ROM_LOAD( "jr.pac-man_8j_11-9-83.8j",    0xa000, 0x2000, CRC(e788dfe2) SHA1(fa705b1ff20846e876b83fb7e1182182b4043759) )
	ROM_LOAD( "jr.pac-man_8k_11-9-83.8k",    0xc000, 0x2000, CRC(86516ff7) SHA1(df063ea4f23efdb7a311dece0068165aa077545f) )
	ROM_LOAD( "pac4eva.6x",                  0x6000, 0x2000, CRC(7e680231) SHA1(8551e0f4e0fe5faab00a122333d370e8d526729d) ) // expansion ROM 2 (engine modules + maze tables; plaintext)
	ROM_LOAD( "pac4eva.8x",                  0xe000, 0x2000, CRC(adfbf49c) SHA1(b8315d3a743765b9f567c287dfb3f5ff23f2a85c) ) // expansion ROM (screens, ghost engine; plaintext)

	ROM_REGION( 0x6000, "gfx1", 0 )   // tiles 0x2000 + sprites 0x4000 (256 shapes)
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



/*************************************
 *
 *  Driver initialization
 *
 *************************************/

void pac4eva_state::init_pac4eva()
{
	/* The encryption PALs garble bits 0, 2 and 7 of the ROMs. The encryption */
	/* scheme is complex (basically it's a state machine) and can only be */
	/* faithfully emulated at run time. To avoid the performance hit that would */
	/* cause, here we have a table of the values which must be XORed with */
	/* each memory region to obtain the decrypted bytes. */
	/* Decryption table provided by David Caldwell (david@indigita.com) */
	/* For an accurate reproduction of the encryption, see jrcrypt.c */
	/* NOTE: the table is all-zero over 0x6000-0x7fff and 0xe000+, which is why */
	/* the two expansion ROMs can be plaintext. */
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
 *  Game driver
 *
 *************************************/

GAME( 2026, pac4eva, 0, pac4eva, pac4eva, pac4eva_state, init_pac4eva, ROT90, "TORNOTLUKIN", "Pac-Man 4 EVA", MACHINE_SUPPORTS_SAVE )
