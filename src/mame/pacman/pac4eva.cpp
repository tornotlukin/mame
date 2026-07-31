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

    THE 3-REGION COORDINATE SYSTEM (why the video code lives here)

    Actors store X in a single byte, which the stock game hides behind a
    scrolling 224px window. Showing the whole 432px maze re-exposes that limit,
    so every actor carries a "region": the maze is three overlapping 256px laps
    and bits 5/6 of the sprite colour byte shift the rendered sprite +/-256px.
    The maze itself is drawn STATIC - sprites must never add the hardware
    scroll, because the game still writes a non-zero scroll value in frightened
    mode which would fling every sprite off the playfield.

    This driver is self-contained: it carries its own tilemap, sprite renderer
    and screen update rather than patching the shared pacman_v.cpp, so stock
    Pac-Man family drivers are untouched and this file survives MAME rebases.

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
		, m_spritext(*this, "spritext")
		, m_sprhi(*this, "sprhi")
	{ }

	void pac4eva(machine_config &config);

	void init_pac4eva();

private:
	void main_map(address_map &map) ATTR_COLD;
	void port_map(address_map &map) ATTR_COLD;

	// video - our own, so the shared pacman_v.cpp is never modified
	TILEMAP_MAPPER_MEMBER(scan_rows);
	TILE_GET_INFO_MEMBER(get_tile_info);
	void mark_tile_dirty(int offset);
	void videoram_w(offs_t offset, uint8_t data);
	void scroll_w(uint8_t data);
	DECLARE_VIDEO_START(pac4eva);
	void draw_sprites(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	optional_shared_ptr<uint8_t> m_spritext;   // 2 extended (software) sprites, slots 8-9
	optional_shared_ptr<uint8_t> m_sprhi;      // per-sprite high bank (code bit7) for the 256-shape set
};



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

void pac4eva_state::main_map(address_map &map)
{
	map(0x0000, 0x3fff).rom();
	map(0x4000, 0x47ff).ram().w(FUNC(pac4eva_state::videoram_w)).share("videoram");
	map(0x4800, 0x4aff).ram();
	map(0x4b00, 0x4b07).ram().share("spritext");   // 2 extended (software) sprites (slots 8,9)
	map(0x4b08, 0x4b11).ram().share("sprhi");      // per-sprite high bank: [0..7]=hw sprites, [8..9]=extended
	map(0x4b12, 0x4fef).ram();
	map(0x4ff0, 0x4fff).ram().share("spriteram");
	map(0x5000, 0x503f).portr("P1");
	map(0x5000, 0x5007).w("latch1", FUNC(ls259_device::write_d0));
	map(0x5040, 0x507f).portr("P2");
	map(0x5040, 0x505f).w(m_namco_sound, FUNC(namco_wsg_device::pacman_sound_w));
	map(0x5060, 0x506f).writeonly().share("spriteram2");
	map(0x5070, 0x5077).w("latch2", FUNC(ls259_device::write_d0));
	map(0x5080, 0x50bf).portr("DSW1");
	map(0x5080, 0x5080).w(FUNC(pac4eva_state::scroll_w));
	map(0x50c0, 0x50c0).w(m_watchdog, FUNC(watchdog_timer_device::reset_w));
	map(0x5100, 0x5100).portr("P3");               // extra simultaneous-player inputs
	map(0x5101, 0x5101).portr("P4");
	// Expansion ROM 3 - a 2K chip decoded into the unused window above the I/O block. Stock
	// hardware decodes nothing past 0x5101, so this costs no stock behaviour. It exists to
	// keep bulk game DATA (fruit tables, and whatever comes next) out of expansion ROM 2,
	// which the maze editor's per-maze data fills as the maze library grows.
	// Expansion ROM 4 - the per-player pac chains. Starts at 0x5180, NOT 0x5100: 0x5100 and
	// 0x5101 are P3's and P4's stick ports mapped just above, and a ROM decoded over them
	// would take the inputs away from two players.
	map(0x5180, 0x57ff).rom();                     // expansion ROM 4 (per-player chains; plaintext)
	map(0x5800, 0x5fff).rom();                     // expansion ROM 3 (2K; plaintext)
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
 *  Video hardware
 *
 *************************************/

TILEMAP_MAPPER_MEMBER(pac4eva_state::scan_rows)
{
	row += 2;
	col -= 2;
	if (col & 0x20)
	{
		// The 4 edge strips (HUD rows) have RAM for only 30 of their 54 cells. The live 30
		// are CENTRED - display cols 0x0C-0x29 (native rows 12-41) map to slots 2-31, an
		// exact budget with no spares; the dead 24 split 12+12 at the screen ends.
		if (row >= 14 && row <= 43)
			return (row - 12) + (((col & 0x3) | 0x38) << 5);
		else
			return 0x77f; // outside the visible area
	}
	else
		return col + (row << 5);
}


TILE_GET_INFO_MEMBER(pac4eva_state::get_tile_info)
{
	int color_index;
	if (tile_index < 1792)
		color_index = tile_index & 0x1f;
	else
		color_index = tile_index + 0x80;

	int code = m_videoram[tile_index] | (m_charbank << 8);
	int attr = (m_videoram[color_index] & 0x1f) | (m_colortablebank << 5) | (m_palettebank << 6);

	tileinfo.set(0, code, attr, 0);
}


void pac4eva_state::mark_tile_dirty(int offset)
{
	if (offset < 0x20)
	{
		/* line color - mark whole line as dirty */
		for (int i = 2 * 0x20; i < 56 * 0x20; i += 0x20)
			m_bg_tilemap->mark_tile_dirty(offset + i);
	}
	else if (offset < 1792)
	{
		/* tiles for playfield */
		m_bg_tilemap->mark_tile_dirty(offset);
	}
	else
	{
		/* tiles & colors for top and bottom two rows */
		m_bg_tilemap->mark_tile_dirty(offset & ~0x80);
	}
}


void pac4eva_state::videoram_w(offs_t offset, uint8_t data)
{
	m_videoram[offset] = data;
	mark_tile_dirty(offset);
}


void pac4eva_state::scroll_w(uint8_t data)
{
	// Widescreen: the whole maze is visible, so the playfield never scrolls. The value the
	// game writes is deliberately DISCARDED - sprite positions are absolute (see draw_sprites).
	for (int i = 2; i < 34; i++)
		m_bg_tilemap->set_scrolly(i, 0);
}


VIDEO_START_MEMBER(pac4eva_state,pac4eva)
{
	save_item(NAME(m_charbank));
	save_item(NAME(m_spritebank));
	save_item(NAME(m_palettebank));
	save_item(NAME(m_colortablebank));
	save_item(NAME(m_flipscreen));
	save_item(NAME(m_bgpriority));
	save_item(NAME(m_irq_mask));
	save_item(NAME(m_interrupt_vector));

	m_charbank = 0;
	m_spritebank = 0;
	m_palettebank = 0;
	m_colortablebank = 0;
	m_flipscreen = 0;
	m_bgpriority = 0;
	m_inv_spr = 0;
	m_xoffsethack = 1;

	m_bg_tilemap = &machine().tilemap().create(
			*m_gfxdecode,
			tilemap_get_info_delegate(*this, FUNC(pac4eva_state::get_tile_info)),
			tilemap_mapper_delegate(*this, FUNC(pac4eva_state::scan_rows)),
			8, 8, 36, 54);

	m_bg_tilemap->set_transparent_pen(0);
	m_bg_tilemap->set_scroll_cols(36);
}


void pac4eva_state::draw_sprites(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	uint8_t *spriteram = m_spriteram;
	uint8_t *spriteram_2 = m_spriteram2;

	// Open the sprite clip to the full rendered bitmap so absolute-positioned actors are
	// never dropped anywhere on the static maze. &= cliprect still bounds it to the screen.
	rectangle spriteclip(0, 36*8-1, 0, 56*8-1);
	spriteclip &= cliprect;

	// One sprite draw, shared by the hardware and extended loops below.
	auto plot = [&] (int code, int color, uint8_t fx, uint8_t fy, int sx, int sy)
	{
		m_gfxdecode->gfx(1)->transmask(bitmap, spriteclip, code, color, fx, fy, sx, sy,
				m_palette->transpen_mask(*m_gfxdecode->gfx(1), color & 0x3f, 0));
	};

	// Absolute Y, plus the 3-region shift: colour bit5 = region B (+256), bit6 = region C (-256).
	// 75 is the render-align constant (-31 + 106); it replaces the stock +0x6a scroll shift so
	// coordinates stay native. The hardware scroll is deliberately NOT added.
	auto sprite_y = [] (uint8_t coord, uint8_t color) -> int
	{
		int sy = coord + 75;
		if (color & 0x20) sy += 256;
		else if (color & 0x40) sy -= 256;
		return sy;
	};

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	// No wraparound duplicate: jrpacman wraps along the other axis (the region system handles
	// it), and the legacy double-draw ghosts sprites parked in the bottom strip.
	for (int offs = m_spriteram.bytes() - 2; offs > 2*2; offs -= 2)
	{
		int sx = 272 - spriteram_2[offs + 1];
		int sy = sprite_y(spriteram_2[offs], spriteram[offs + 1]);

		uint8_t fx = spriteram[offs] & 1;
		uint8_t fy = spriteram[offs] & 2;

		int color = (spriteram[offs + 1] & 0x1f) | (m_colortablebank << 5) | (m_palettebank << 6);
		// Colour bit7 promotes this one sprite to the high half of the current bank; sprhi
		// supplies bit 8 of the code, reaching the full 256-shape set. In-game actors leave
		// sprhi clear so they stay in shapes 0-127.
		int code = (spriteram[offs] >> 2)
				| ((m_spritebank | ((spriteram[offs + 1] >> 7) & 1)) << 6)
				| ((m_sprhi ? (m_sprhi[offs >> 1] & 1) : 0) << 7);

		plot(code, color, fx, fy, sx, sy);
	}

	/* In the Pac Man based games (NOT Pengo) the first two sprites must be offset */
	/* one pixel to the left to get a more correct placement */
	for (int offs = 2*2; offs >= 0; offs -= 2)
	{
		int sx = 272 - spriteram_2[offs + 1];
		int sy = sprite_y(spriteram_2[offs], spriteram[offs + 1]);

		uint8_t fx = spriteram[offs] & 1;
		uint8_t fy = spriteram[offs] & 2;

		int color = (spriteram[offs + 1] & 0x1f) | (m_colortablebank << 5) | (m_palettebank << 6);
		int code = (spriteram[offs] >> 2)
				| ((m_spritebank | ((spriteram[offs + 1] >> 7) & 1)) << 6)
				| ((m_sprhi ? (m_sprhi[offs >> 1] & 1) : 0) << 7);

		plot(code, color, fx, fy, sx, sy + m_xoffsethack);
	}

	// The 2 EXTENDED software sprites (slots 8,9) from spritext at 0x4b00, 4 bytes each:
	// [img, colour, sy_src, sx_src]. Same alignment, region, bank and flip rules as the
	// hardware sprites, giving 4 players + 5 ghosts + fruit = 10 on screen.
	if (m_spritext)
	{
		for (int e = 1; e >= 0; e--)
		{
			uint8_t img = m_spritext[e*4+0];
			if (!img) continue;                 // img 0 = inactive
			uint8_t col = m_spritext[e*4+1];

			int sy = sprite_y(m_spritext[e*4+2], col);
			int sx = 272 - m_spritext[e*4+3];

			int color = (col & 0x1f) | (m_colortablebank << 5) | (m_palettebank << 6);
			int code  = (img >> 2)
					| ((m_spritebank | ((col >> 7) & 1)) << 6)
					| ((m_sprhi ? (m_sprhi[8 + e] & 1) : 0) << 7);

			plot(code, color, img & 1, img & 2, sx, sy);
		}
	}
}


uint32_t pac4eva_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	if (m_bgpriority != 0)
		bitmap.fill(0, cliprect);
	else
		m_bg_tilemap->draw(screen, bitmap, cliprect, TILEMAP_DRAW_OPAQUE, 0);

	if (m_spriteram != nullptr)
		draw_sprites(screen, bitmap, cliprect);

	if (m_bgpriority != 0)
		m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 0);

	return 0;
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
	m_screen->set_screen_update(FUNC(pac4eva_state::screen_update));

	MCFG_VIDEO_START_OVERRIDE(pac4eva_state,pac4eva)
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
	ROM_LOAD( "jr.pac-man_8d_11-9-83.8d",    0x0000, 0x2000, CRC(e99e4500) SHA1(dce7731c63d6d55bcb59f6aa38f216a0e0fee42e) )
	ROM_LOAD( "jr.pac-man_8e_11-9-83.8e",    0x2000, 0x2000, CRC(cecd969f) SHA1(d7bdc5730db9dc6689227e806b778c43c00cea48) )
	ROM_LOAD( "jr.pac-man_8h_11-9-83.8h",    0x8000, 0x2000, CRC(005a0c5d) SHA1(c3c08d526560fc416482c97e0cff6ba8855c3b9c) )
	ROM_LOAD( "jr.pac-man_8j_11-9-83.8j",    0xa000, 0x2000, CRC(8f10e62a) SHA1(c98b5655a61bb9cbca0f3296308a6467128e7e3c) )
	ROM_LOAD( "jr.pac-man_8k_11-9-83.8k",    0xc000, 0x2000, CRC(4939f690) SHA1(5fa3e8baa7db4d762606a513510442b765be3a90) )
	ROM_LOAD( "pac4eva.5y",                  0x5180, 0x0680, CRC(05ce6ee7) SHA1(040d3e31a3ad840c199feedeb9c337665ee785f6) ) // expansion ROM 4 (per-player chains; plaintext)
	ROM_LOAD( "pac4eva.5x",                  0x5800, 0x0800, CRC(436c5e24) SHA1(77937c9c5dac268bb2fbffc2e44339c425a8b2d8) ) // expansion ROM 3 (bulk game data; plaintext)
	ROM_LOAD( "pac4eva.6x",                  0x6000, 0x2000, CRC(7343cdd4) SHA1(9289fe93045981f8f1d7803ac1ad9631d137d2ae) ) // expansion ROM 2 (engine modules + maze tables; plaintext)
	ROM_LOAD( "pac4eva.8x",                  0xe000, 0x2000, CRC(ffb431c1) SHA1(2d8a5d1d3a53a42a41bf1f19467e31506909694c) ) // expansion ROM (screens, ghost engine; plaintext)

	ROM_REGION( 0x6000, "gfx1", 0 )   // tiles 0x2000 + sprites 0x4000 (256 shapes)
	ROM_LOAD( "jr.pac-man_2c_11-9-83.2c",    0x0000, 0x2000, CRC(a624f5cb) SHA1(90809d9d30df183461c0c40f2da941a21fec6d5c) ) /* tiles (512) */
	ROM_LOAD( "jr.pac-man_2e_11-9-83.2e",    0x2000, 0x4000, CRC(18be94ea) SHA1(adcaed4ec0a7119307ce908c3cc83122c42fcb99) ) /* sprites (256) */

	ROM_REGION( 0x0120, "proms", 0 )
	ROM_LOAD_NIB_LOW ( "a290-27axv-bxhd.9e", 0x0000, 0x0100, CRC(029d35c4) SHA1(d9aa2dc442e9ac36cf3c346b9fb1aa745eaf3cb8) ) /* color palette (low bits) */
	ROM_LOAD_NIB_HIGH( "a290-27axv-cxhd.9f", 0x0000, 0x0100, CRC(eee34a79) SHA1(7561f8ccab2af85c111af6a02af6986eb67503e5) ) /* color palette (high bits) */
	ROM_LOAD( "a290-27axv-axhd.9p",          0x0020, 0x0100, CRC(0b897f88) SHA1(340396865cfa78c2343bf659bfc7e5557bde2414) ) /* color lookup table */

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
