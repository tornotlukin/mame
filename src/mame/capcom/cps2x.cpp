// license:BSD-3-Clause
// copyright-holders:Paul Leaman, Andreas Naive, Nicola Salmoria
/***************************************************************************

    cps2x.cpp -- the CPS-2X driver

    A self-contained copy of the CPS2 driver (src/mame/capcom/cps2.cpp, MAME
    0.289) carrying the mods that stock cps2.cpp does not:

      - the 4-player 2v2 tag mod for the VS series (P3/P4 inputs, the on-point
        partner mux, the 1v1/2v2 Play Mode): xmvsf2v2, mshvsf2v2, mvsc2v2;
      - the widescreen 4-live Duo variant: mvscduo;
      - the CPS-2X expanded board (expansion chips, extended palette pages,
        the 32MB QSound sample space, the telemetry NVRAM): mvscextra (dev
        set, Lua loader) and mvscextrab (the baked release).

    Every machine here is a clone of a stock cps2.cpp set. The board code
    (state class, video, maps, configs) is cps2.cpp's, renamed to
    cps2x_state; it still derives from the stock cps_state (cps1.h). The
    QSound chip is qsoundx (qsoundx.h), a copy of the stock HLE device, so
    no stock file is edited. To add the driver to a MAME tree: copy
    cps2x.cpp, qsoundx.cpp and qsoundx.h into src/mame/capcom/ and add the
    cps2x.cpp block to src/mame/mame.lst.

    The original cps2.cpp notes follow.

***************************************************************************/

/***************************************************************************

Driver by Paul Leaman (paul@vortexcomputing.demon.co.uk)

Thanks to Andreas Naive and Nicola Salmoria for understanding the
encryption mechanism.

Thanks to Razoola, Ange 'Crashtest' Albertini and the CPS2 decryption team
for their initial work on extracting decrypted code.


  Capcom System 2
  ===============

CPS2 Hardware Overview
Capcom, 1993-2004

From 1993 to 2004 Capcom produced a generic system known as CPS2 (Capcom Play System 2). It
comprises a base board (known as the A-Board) and a top board (known as the B-Board). Both were
housed in separate plastic boxes and fitted together via four multi-pin connectors.
The boxes are colour-coded for release in specific country regions....
Green  - Japan
Blue   - U.S.A. and Europe
Orange - South America
Grey   - Asia
Yellow - All (Rent version; can be hired by operators for testing)

Grey and orange B-Boards require an A-Board with a matching colour to work. Green A/B and Blue A/B
boards are interchangeable. Yellow boards will fit any A-Board since they were used for renting to
operators for testing purposes in the hope that they would buy it.

The first game produced on this system was "Super Street Fighter II The New Challengers". The last
game (so far) is "Hyper Street Fighter II : The Anniversary Edition". All up, around 50 unique
games were developed on CPS2 hardware, with dozens of releases in different regions, totalling over
170 variations. Over the years Capcom have produced a lot of games using the same theme, making
numerous spin-offs of their titles with varying subtitles in an effort to prolong the life of the
aging CPS2 hardware (one would assume).

The base board contains the main CPU (a 68000), RAM, graphic generating hardware, sound hardware
(Q-Sound and Kabuki taken from CPS1) and I/O hardware. The top board contains the software
(EPROMS etc) and some custom ICs used to generate backgrounds and sprites.
The CPS2 hardware is very different from most other generic arcade systems in that it uses some
very complex encryption of the main program to combat bootleggers. The decryption key is held in
some SRAM inside one of the custom IC's and powered by a battery. If the battery dies the system
kills itself, showing only a blue or green screen on power-up. There are no known simple fixes so
far so it is then unusable, but can be returned to Capcom for a repair, providing the security
stickers have not been tampered with and the plastic box has not been opened.
The encryption is so involved that there were no known bootlegs of any of the CPS2 games.

The sound CPU is a standard Z80. On some PCBs a custom Z80 is used instead (called a "Kabuki"). This
is also encrypted and is powered by a battery. When this battery dies, the chip works as a regular
Z80 and can not decrypt the program so there is no sound, but the game continues to work. However,
in all CPS2 games, Capcom chose not to use the Kabuki encryption since none of the CPS2 games have
an encrypted sound program. There is a jumper pad on the PCB next to the Z80 (JP3). When this is
shorted, it sets a pin on the Kabuki to run in encrypted mode, but it has been found not shorted on
all CPS2 games so far. The Kabuki or a regular Z80 has been found in use for the sound CPU, so it
is possible Capcom are using the Kabuki chips from old stock as regular Z80 CPUs.

PCB Layouts
-----------

A-Board
-------
CAPCOM 93646A-6
   |----------------------------------------------------------------------------------------------|
   |SW3 SW2 SW1                    |-----------------------|     |-----------------------|        |
|--|                               |-----------------------|     |-----------------------|     CN6|
|                                8464    93C46(2)      CN3        CN4                             |
|                                                                                                 |
|                                  PAL1               |---------|                                 |
|                                                     |CAPCOM-Q1|                                 |
|                                  PAL2               |DL 1425  |               |---------|       |
|J                                                    |DSP-16A  |               |CAPCOM   |       |
|A              5863                                  |(PLCC84) |               |CPS-B-21 |       |
|M                                  |---------------| |---------|               |DL-0921  |       |
|M              5863                |DL-030P(or Z80)|                           |(QFP160) |       |
|A                                  |---------------|                           |---------|       |
|                                                    60MHz                                        |
|            |---------|            8MHz                                                          |
|            |CAPCOM   |                                                                          |
|            |DL-1123  |                                                 PAL3                     |
|    93C46(1)|I/O      |                                  |---------|                             |
|            |(QFP136) |                                  |CAPCOM   |                             |
|--|         |---------|                                  |CPS-A-01 |              |---------|    |
   |                                                      |DL-0311  |              |CAPCOM   |    |
   |                                                      |(QFP160) |              |DL-1625  |    |
   |                                                      |---------|   16MHz      |SPB      |    |
   |                                                                               |(QFP128) |    |
  |-|                                                                              |---------|    |
  | |                                                                                             |
  | |                                    M51953B                                                  |
  | |CN5    LM833    TC9176P   LM833                                         |-------|            |
  | |       LM833              NE5532                                        |CAPCOM |            |
  | |       LM833              TDA1543                 PAL4    HM658128      |DL-2227|  HM514260  |
  |-|                                                                        |DRC    |            |
   |            BATTERY     TC9185P                    PAL5    HM658128      |(QFP64)|  HM514260  |
  |- CNL                                     CN1                         CN2 |-------|            |
   |         TA8225L               |-----------------------|     |-----------------------|        |
  |- CNR                           |-----------------------|     |-----------------------|        |
   |----------------------------------------------------------------------------------------------|
Notes:
      5863      - Sony CXK5863BP-35 8k x8 SRAM (DIP28)
      8464      - Fujitsu MB8464A-10L 8k x8 SRAM (SOP28)
      HM514260  - Hitachi HM514260AJ8 256k x 16 DRAM (SOJ40)
      HM658128  - Hitachi HM658128ADFP-10 128k x8 PSRAM (SOP32)
      M5195B    - Mitsubishi Electric Corp. M5195B Voltage Detection and System Reset IC (SIP5)
      LM833     - National Semiconductor LM833 Dual Audio Operational Amplifier (DIP8)
      TDA1543   - Philips TDA1543 Dual 16-bit DAC (DIP8)
      NE5532    - Philips NE5532 Internally-compensated Dual Low Noise Operational Amplifier (DIP8)
      TC9176P   - Toshiba TC9176 (purpose unknown, DIP16)
      TC9185P   - Toshiba TC9185P Electronic Volume Control IC (DIP20)
      TA8225L   - Toshiba TA8225L 45W BTL Audio Amplifier (ZIP17)
      93C46(1)  - Atmel 93C46 EEPROM (SOIC8, tied to the custom I/O chip)
      93C46(2)  - Atmel 93C46 EEPROM (SOIC8, tied to the Kabuki chip)
      SW1       - Test Switch
      SW2       - Volume Up
      SW3       - Volume Down
      CN1/2/3/4 - 96 Pin Connectors for connection of top B-board
      CN5       - 34 Pin Connector used for (generally) extra kick buttons on fighting games
      CN6       - 2 Pin Fan connector
      CN L/R    - RCA Connectors for Left/Right Audio when QSound (stereo output) is enabled
      PAL1      - MMI PAL16L8 (DIP20, stamped 'D8L1')
      PAL2      - MMI PAL16L8 (DIP20, stamped 'BGSA4')
      PAL3      - MMI PAL16R4 (DIP20, stamped 'BGSA5')
      PAL4      - MMI PAL16L8 (DIP20, stamped 'BGSA1')
      PAL5      - MMI PAL16L8 (DIP20, stamped 'BGSA2')
      VSync     - 59.6388Hz
      HSync     - 15,444.5kHz

      Custom IC's -
                   DL-030P - KABUKI Custom encrypted Z80, running at 8.000MHz, manufactured by VLSI
                             Technology (DIP40), INT frequency ~249.932Hz-250.070Hz.
                             On most PCB's this is a regular Zilog Z80 (Z0840008PSC)
                   DL-1425 - CAPCOM-Q1 QSound Processor, DSP-16A (C) 92 AT&T (PLCC84)
                             * pin 33 - CKO (O): moving very fast, connected to a logic chip on the A board
                             * pin 58 - ICK (I/O): 60MHz
                             * pin 59 - OCK (I/O): 60MHz
                             * pin 67 - CKI (I): 60MHz
                             * pin 71 - INT (I): erratic, active during qsound writes
                   DL-0921 \
                   DL-0311 / CPS-A/B Graphics Processors (QFP160)
                   DL-1625 - DRAM interface (QFP128)
                   DL-2227 - DRAM Refresh Controller (QFP64)
                   DL-1123 - I/O Controller (QFP136)

      Connector Pinouts -

                       JAMMA Connector                                     Extra Button Connector
                       ---------------                                     ----------------------
                    PART SIDE    SOLDER SIDE                                     TOP    BOTTOM
                ----------------------------                             --------------------------
                      GND  01    A  GND                                      GND  01    02  GND
                      GND  02    B  GND                                      +5V  03    04  +5V
                      +5V  03    C  +5V                                     +12V  05    06  +12V
                      +5V  04    D  +5V                             Configurable  07    08  Configurable
                       NC  05    E  NC                              Configurable  09    10  Configurable
                     +12V  06    F  +12V                            Configurable  11    12  Configurable
                           07    H                                  Configurable  13    14  Configurable
           Coin Counter 1  08    J  NC                              Configurable  15    16  Configurable
             Coin Lockout  09    K  Coin Lockout                    Configurable  17    18  Configurable
               Speaker (+) 10    L  Speaker (-)                     Configurable  19    20  Configurable
                       NC  11    M  NC                              Configurable  21    22  Configurable
                Video Red  12    N  Video Green                     Configurable  23    24  Configurable
               Video Blue  13    P  Video Composite Sync            Configurable  25    26  Configurable
             Video Ground  14    R  Service Switch                  Configurable  27    28  Configurable
                     Test  15    S  NC                               Volume Down  29    30  Volume UP
                   Coin A  16    T  Coin B                                   GND  31    32  GND
           Player 1 Start  17    U  Player 2 Start                           GND  33    34  GND
              Player 1 Up  18    V  Player 2 Up
            Player 1 Down  19    W  Player 2 Down
            Player 1 Left  20    X  Player 2 Left                Pins 07 to 29 can be configured to anything
           Player 1 Right  21    Y  Player 2 Right               as games require. This includes coin inputs,
        Player 1 Button 1  22    Z  Player 2 Button 1            coin lockouts, joysticks and buttons. There
        Player 1 Button 2  23    a  Player 2 Button 2            are at least 2 known configurations seen in
        Player 1 Button 3  24    b  Player 2 Button 3            CPS-2 games released so far (see below).
        Player 1 Button 4  25    c  Player 2 Button 4
                       NC  26    d  NC
                      GND  27    e  GND
                      GND  28    f  GND

Note that only some games use the player 1 and 2 button 4 output on the JAMMA connector.
Most games that require the use of button 4 get it from the extra button connector.

Known Extra Button Connector configurations -

             Most fighting type games                       More than 2 Player games
             ------------------------                       ------------------------
                   TOP    BOTTOM                                  TOP    BOTTOM
           ----------------------------                   ----------------------------
               GND  01    02  GND                             GND  01    02  GND
               +5V  03    04  +5V                             +5V  03    04  +5V
              +12V  05    06  +12V                           +12V  05    06  +12V
                NC  07    08  NC                    3Up Coin Lock  07    08  4Up Coin Lock
      2Up Button 6  09    10  NC                         3Up Coin  09    10  4Up Coin
                NC  11    12  NC                        3Up Start  11    12  4Up Start
                NC  13    14  NC                           3Up Up  13    14  4Up Up
      1Up Button 6  15    16  NC                         3Up Down  15    16  4Up Down
      1Up Button 5  17    18  NC                         3Up Left  17    18  4Up Left
      1Up Button 4  19    20  NC                        3Up Right  19    20  4Up Right
      2Up Button 4  21    22  NC                     3Up Button 1  21    22  4Up Button 1
      2Up Button 5  23    24  NC                     3Up Button 2  23    24  4Up Button 2
                NC  25    26  NC                     3Up Button 3  25    26  4Up Button 3
                NC  27    28  NC                     3Up Button 4  27    28  4Up Button 4
       Volume Down  29    30  Volume UP               Volume Down  29    30  Volume UP
               GND  31    32  GND                             GND  31    32  GND
               GND  33    34  GND                             GND  33    34  GND

Spinners -
There are 2 known games which use spinners, Puzz Loop 2 and Eco Fighters.
Puzz Loop 2 can come with an extra PCB that sits between the JAMMA harness and A-Board.
The spinners plug into this board and there is an option in the games settings to enable
it. If the extra PCB board is missing its not a problem because the game is still fully
playable using normal joysticks.
Eco Fighters has a special limited version of the game (not currently dumped) that uses
spinners. This version of the game did not support joysticks (only the normal version of
the game does). Not much more is known about how the spinners used here hook up to the
system or if the hardware required is the same as that used with Puzz Loop 2.


B-Boards
--------
CAPCOM 93646B-4  \  There are small variations between board revisions; changed
CAPCOM 93646B-6   | PALs, alternative location for an EEPROM, extra connectors
CAPCOM 93646B-7  /  (CN9) and other minor diferences.
 |-------------------------------------------------------------------------|
 |       CN3|-----------------------|     |-----------------------|CN4     |
 |          |-----------------------|     |-----------------------|        |
|-|      CN5|-----------------------|     |-----------------------|CN6     |
| |                                                                        |
| |  ROM1.1A                 ROM11.1E                                      |
| |           93C46                        ROM13.1J        ROM17.1M        |
| |  ROM2.2A                 ROM12.2E                                      |
| |                                        ROM14.2J        ROM18.2M        |
| |CN7                                                                     |
| |                                        ROM15.3J        ROM19.3M        |
| |                                                                        |
| | 8464     |---------|   |---------|     ROM16.4J        ROM20.4M        |
| |          |CAPCOM   |   |CAPCOM   |                                     |
| | 8464 CN9 |DL-1827  |   |DL-1525  |                                     |
|-|          |CIF      |   |SPA      |                                     |
 |           |(QFP160) |   |(QFP208) |                                     |
 |           |---------|   |---------|                                     |
 |                                                                         |
 |                                                                         |
 |                                                                         |
 |   ROM3.6A        ROM7.6D     |---------|   |---------|   |---------|    |
 |                              |CAPCOM   |   |CAPCOM   |   |CAPCOM   |    |
 |                              |DL-1727  |   |DL-2027  |   |DL-1927  |    |
 |   ROM4.7A        ROM8.7D     |MIF      |   |CGD      |   |CGA      |    |
 |                              |(QFP160) |   |(QFP120) |   |(QFP120) |    |
 |                              |---------|   |---------|   |---------|    |
 |   ROM5.8A        ROM9.8D                                                |
 |                                                                         |
 |                                    PAL1    PAL3               BATTERY   |
 |   ROM6.9A        ROM10.9D                                               |
 |                                    PAL2                                 |
 |                    CN1                         CN2                      |
 |          |-----------------------|     |-----------------------|        |
 |          |-----------------------|     |-----------------------|        |
 |-------------------------------------------------------------------------|
Notes:
      8464      - Fujitsu MB8464A-10L 8k x8 SRAM (SOP28)
      93C46     - Atmel 93C46 EEPROM (SOIC8, not populated on revision -7 board)
      CN1/2/3/4 - 96 Pin Connectors for connection to lower A-board (connectors below PCB)
      CN5/6     - 96 Pin Connectors for connection to optional daughter boards used by some games
                  (connectors above PCB)
      CN7       - 64 Pin Connector for connection of optional Communication Board
      CN9       - 6 Pin JTAG Connector (only on newer B-board revisions), it's used for reprogramming
                  custom IC's (probably encryption key information)
      PAL1      - MMI PAL16L8 (DIP20, stamped 'BGSB1')
      PAL2      - MMI PAL16L8 (DIP20, stamped 'BGSB2')
      PAL3      - MMI PAL16L8 (DIP20, stamped 'BGSB3F' on rev -4 board, stamped 'BGSB3G' on rev -7
                  board)

      Custom IC's -
                   DL-1827 CIF - Fujitsu CG24 series gate array (QFP160)
                   DL-1525 SPA - Motorola H4C series model 057 gate array with 68000 CPU,
                                 Running at 16MHz. (QFP208)
                   DL-1727 MIF - Fujitsu CG24 series gate array (QFP120)
                   DL-2027 CGD - Fujitsu CG24 series gate array (QFP120)
                   DL-1927 CGA - Fujitsu CG24 series gate array (QFP120)

      ROMs -
            Note, the ROM names shown on the above layout are generic. Each EPROM on every game has
            a unique sticker attached to it. All of the mask ROMs are also stamped with unique names
            for each game. The amount of EPROMs/mask ROMs used also differs per game, depending on
            requirements. The PCB is wired for certain sized ROMs by default, but via jumpers they
            can be reconfigured to allow accepting other sized devices.

                                                                           Some example ROM names
                                                                  (see the source below for full name details)
                                                                         /---------------------------\
            Location  Device type and size               Use               XMen COTA    Puzz Loop 2
            ------------------------------------------------------------------------------------------
            1.1A      27C010 (1M) or 27C1000 (1M)        Sound Program     XMN_01.1A    PL2_01.1A
            2.2A      27C010 (1M) or 27C1000 (1M)        Sound Program     XMN_02.2A    -
            3.6A      27C4096 (4M)                       Main Program      XMNE_03E.6A  PL2J_03.6A
            4.7A      27C4096 (4M)                       Main Program      XMNE_04E.7A  PL2J_04.7A
            5.8A      27C4096 (4M)                       Main Program      XMNE_05.8A   PL2J_05.8A
            6.9A      27C4096 (4M)                       Main Program      XMNE_06.9A   PL2J_06.9A
            7.6D      27C4096 (4M)                       Main Program      XMNE_07.6D   -
            8.7D      27C4096 (4M)                       Main Program      XMNE_08.7D   -
            9.8D      27C4096 (4M)                       Main Program      XMNE_09.8D   -
            10.9D     27C4096 (4M)                       Main Program      XMNE_10.9D   -
            11.1E     HN624316 (16M) or KM23C32000 (32M) QSound Samples    XMN_11M.1E   \
            12.2E     HN624316 (16M) or KM23C32000 (32M) QSound Samples    XMN_12M.2E   |
            13.1J     HN624316 (16M) or KM23C32000 (32M) Graphics          XMN_13M.1J   |
            14.2J     HN624316 (16M) or KM23C32000 (32M) Graphics          XMN_14M.2J   |
            15.3J     HN624316 (16M) or KM23C32000 (32M) Graphics          XMN_15M.3J   |
            16.4J     HN624316 (16M) or KM23C32000 (32M) Graphics          XMN_16M.4J   |  Located
            17.1M     HN624316 (16M) or KM23C32000 (32M) Graphics          XMN_17M.1M   |  on SIMMs
            18.2M     HN624316 (16M) or KM23C32000 (32M) Graphics          XMN_18M.2M   |
            19.3M     HN624316 (16M) or KM23C32000 (32M) Graphics          XMN_19M.3M   |
            20.4M     HN624316 (16M) or KM23C32000 (32M) Graphics          XMN_20M.4M   /

            Capcom have a unique ROM naming system for CPS-2 games as mentioned above. A typical ROM
            name is 'SSFA 03B' and is clearly printed on the sticker of each ROM. In most cases stickers
            have the ROM details over 2 lines, in this case 'SSFA' would be on the first line and '03B'
            on the second line. Each part of this label name is detailed below...

            SSF -  The game title shortened to 3 characters, this game is 'Super Street Fighter II'.

            A   -  The region of the game, in this case 'Asia'. Known regions are...
                   J = Japan          E = ETC (World and Euro)
                   U = USA            A = Asia
                   H = Hispanic       N = Oceania
                   B = Brazil         O = Found in yellow rent versions

            03  -  Under each DIP socket is a large white number. The number on the sticker matches
                   this number as a location identifier.

            B   -  This is the revision of the ROM software. When a game is first released the
                   initial revision will not be noted on the label. During production, the software
                   may be updated and the sticker will then have the letter 'A/B/C/D/E' etc
                   appended to the end to denote a changed revision of the software for that particular
                   ROM. From the example we can see this ROM is the 3rd revision since release.
                   When the software is revised, not all ROMs have to be updated, in many cases only
                   some of the ROMs have a revision update, and the other ROMs will remain the same.


ROM Daughterboards -
                    As well as the 3 daughter boards shown below, there are other variations that
                    exist, but due to lack of information they are not documented here.

CAPCOM 93661G-2
|-------------------------------------------------------------------------|
|                                                                         |
|          |-----------------------|     |-----------------------|        |
|       CN1|-----------------------|     |-----------------------|CN2     |
|                                                                         |
|                                                                         |
| PAL.1A     ROMQ1.1C      ROMQ5.1D      ROM21.1E      ROM25.1F           |
| PAL.2A                                                                  |
|            ROMQ2.2C      ROMQ6.2D      ROM22.2E      ROM26.2F           |
|  93C46                                                                  |
|            ROMQ3.4C      ROMQ7.4D      ROM23.4E      ROM27.4F           |
|                                                                         |
| Jumpers    ROMQ4.5C      ROMQ8.5D      ROM24.5E      ROM28.5F           |
|                                                                         |
|                                                                         |
|-------------------------------------------------------------------------|
Notes:
      This board is known to be used with some versions of "Street Fighter Zero 2" but not all.
      When it is used with this game, only 4 graphics ROMs are on this board (ROM21 to ROM24),
      all the others are on the B-board.

      CN1/2    - 96 Pin Connectors for connection to B-Board (the connectors are below the PCB)
      PAL.1A   - MMI PAL16L8 (not populated)
      PAL.2A   - MMI PAL16L8 (DIP20, stamped 'BGSG2B')
      93C46    - Atmel 93C46 EEPROM (SOIC8, not populated)
      Jumpers  - 16 Jumper pads
      ROMs     - ROMQ1 to ROMQ8 are HN62344
                 ROM21 to ROM28 are HN624116

CAPCOM 93646C-3
|-------------------------------------------------------------------------|
|            93C46                                                        |
|          |-----------------------|     |-----------------------|        |
|       CN1|-----------------------|     |-----------------------|CN2     |
|                                                                         |
|                                                                         |
|                ROM59.4D       ROM69.4J       ROM79.4M       ROM89.4P    |
|                                                                         |
|                ROM60.5D       ROM70.5J       ROM80.5M       ROM90.5P    |
|                                                                         |
|  ROM51.6A      ROM61.6D       ROM71.6J       ROM81.6M       ROM91.6P    |
|                                                                         |
|  ROM52.7A      ROM62.7D       ROM72.7J       ROM82.7M       ROM92.7P    |
|                                                                         |
|  ROM53.8A      ROM63.8D       ROM73.8J       ROM83.8M       ROM93.8P    |
|                                                                         |
|  ROM54.9A      ROM64.9D       ROM74.9J       ROM85.9M       ROM94.9P    |
|                                                                         |
|  ROM55.10A     ROM65.10D      ROM75.10J      ROM85.10M      ROM95.10P   |
|                                                                         |
|  ROM56.11A     ROM66.11D      ROM76.11J      ROM86.11M      ROM96.11P   |
|                                                                         |
|  ROM57.12A     ROM67.12D      ROM77.12J      ROM87.12M      ROM97.12P   |
|                                                                         |
|  ROM58.13A     ROM68.13D      ROM78.13J      ROM88.13M      ROM98.13P   |
|                                                                         |
|-------------------------------------------------------------------------|
Notes:
      This board is known to be used with some yellow rent versions of games but not all. When
      it is used, no sound or graphics ROMs are used on the B-board.

      CN1/2 - 96 Pin Connectors for connection to B-Board (the connectors are below the PCB)
      93C46 - Atmel 93C46 EEPROM
      ROMs  - All sockets are for 27C4096 devices (ROM51 to ROM58 are for sound and
              ROM59 to ROM98 are for graphics.

CAPCOM 00716C-3
|-------------------------------------------------------------------------|
|                    CN1                            CN2                   |
|          |-----------------------|     |-----------------------|        |
|          |-----------------------|     |-----------------------|        |
|                                                                         |
|           74LS157   74LS157                                             |
|                                                                         |
|             SIMM5                                SIMM1                  |
|                                                                         |
|                                                  SIMM2                  |
|                              74LS04                                     |
|                                                  SIMM3                  |
|                                                                         |
|             SIMM6            M51953              SIMM4                  |
|                                                                         |
|-------------------------------------------------------------------------|
Notes:
      M5195B          - Mitsubishi Electric Corp. M5195B Voltage Detection & System Reset IC (SIP5)
                        (May not be populated on some boards)
      CN1/2           - 96 Pin Connectors for connection to B-Board (the connectors are below the PCB)
      SIMM1/2/3/4/5/6 - Generic 72-pin SIMM sockets (as used on early to mid 90's PC motherboards)
                        The SIMMs themselves are custom. They're not RAM sticks, but instead hold
                        16MBit FlashROMs of type Fujitsu 29F016 TSOP48. Some are standard pinout,
                        some are reverse pinout. This is done to allow for easy PCB wiring on the
                        SIMMs. So far, only SIMMs 1, 3 & 5 are used. 1 & 3 are single sided, have 4
                        FlashROMs on them and hold graphics data that was previously located on the
                        B-Board at locations 1J-4J & 1M-4M.
                        SIMM 5 & 6 can come in two varieties. The more common ones are double-sided
                        and can be populated with up to 8x 16MBit TSOP48 FlashROMs, but only have 2
                        FlashROMs on them (one on each side) and hold the QSound samples that were
                        previously located on the B-Board at locations 1E & 2E.
                        The other type is also double sided and holds up to 8x 16MBit TSOP56 FlashROMs,
                        and again has only 2 positions populated (both on the same side). Either type
                        of QSound SIMM can be used but the data is interleaved differently because of
                        the position of the FlashROM on the SIMM.
                        So far, the data held on the SIMMs is equal to or less than the ROM capacity of
                        the sockets on the B-Board, so the use of SIMMs is a mystery.
                        Some possible explanations are their use is a cost-cutting measure, or they're
                        more easily sourced from the supplier rather than using older 42-pin mask ROMs.
                        Another possibility is they are being re-used from left-over CPS3 boards, since
                        they're identical and are easily re-programmable. In comparision, the GFX SIMMs
                        are the same type as used in CPS3 boards for the main program and the QSound
                        SIMMs are the same type as used in CPS3 boards for the GFX data, but are only
                        populated with 2 FlashROMs.

                        Example SIMM Layout -
                          |----------------------------------------------------|
                          |                                                    |
                          |   |-------|   |-------|   |-------|   |-------|    |
                          |   |Flash_A|   |Flash_B|   |Flash_C|   |Flash_D|    |
                          |   |-------|   |-------|   |-------|   |-------|    |
                          |-                                                   |
                           |-------------------------/\------------------------|
                           Notes:
                                  Flash_A & Flash_C and regular pinout type Fujitsu 29F016A-90PFTN and
                                  for the TSOP56 SIMMs, the FlashROMs are type HN98F1600T10
                                  Flash_B & Flash_D are reverse pinout (Fujitsu 29F016A-90PFTR)
                                  and are mounted upside down also so that pin1 lines up with the normal
                                  pinout of FlashROMs A & C.
                                  For the TSOP48 QSound SIMMs, the 2 FlashROMs are populated at location D
                                  and C only. C is also located on the other side of the SIMM.
                                  For the TSOP56 QSound SIMMs, the 2 FlashROMs are populated at location A
                                  and C only. Both FlashROMs are located on the same side of the SIMM.


Q-Sound Amp board

CAPCOM CPB-001A
|--------------------|
| B Rch Lch          |
|                    |
|      IC1           |
|                    |
| IC3      IC2       |
|                    |
|                    |
|                    |
|                    |
|       ||||         |
|--------------------|

Notes:
      2 RCA cables from A-board CN L/R go in at the top. The audio goes through
      Mitsumi MM1326 series "Q Expander" ICs responsible for creating the 3D stereo
      effect. The 4-pin connector at the bottom goes to the cabinet speakers.

      IC1 - don't know yet which chip is which
      IC2 - "
      IC3 - "


Communication Board

TOURNAMENT CAPCOM 93656D-3
|-------|-|--|-|----------------------------------------------------------|
|       |-|  |-|                                                 BATTERY  |
|      SCN1  SCN2          SCN3                                           |
|                                                                         |
|                                                                         |
|                                                                 8464    |
|    MAX232                                                               |
|                                                                         |
|    PAL                                                                  |
|                                                                         |
|                                                                         |
|    D71051C                                                              |
|                                                                         |
|                                         93C46                           |
|                                                                         |
|-----------------------------------------|              CN1              |
                                          |-------------------------------|
Notes:
      There is sufficient space next to the B-Board to enable this board to plug into the B-Board
      into CN7 and still be fully enclosed inside the housing. The housing has holes in it to allow
      the TX, RX and Register connectors to be accessed without opening the case.
      This board is known to be used with "Super Street Fighter II : The Tournament Battle" and some
      yellow rent boards also have this daughter board attached.

      SCN1    - Network Data IN
      SCN2    - Network Data OUT
      SCN3    - 8 Pin location for a 'Register' device, not populated (possibly for an online
                register of high scores)
      CN1     - 64 Pin Connector for connection to B-Board
      8464    - Fujitsu MB8464A-10L 8k x8 SRAM (SOP28)
      MAX232  - Maxim MAX232CPE Dual EIA-232 Driver/Receiver (DIP16)
      PAL1    - MMI PAL16L8 (DIP20, stamped 'SFSRD')
      D71051C - NEC uPD71051C Serial Control Unit (DIP28)
      93C46   - Atmel 93C46 EEPROM (SOIC8)
      BATTERY - This is a location for a battery but it is not populated

***************************************************************************

Known problems with this driver.

  - The network adapter used in Super Street Fighter II: The Tournament Battle is
    not currently emulated though the ports it uses are setup in the memory map.

  - Giga Wing's attract mode seems to lose sync with music. The problem seems to
    happen due to gfx drawing slowing to much when screen colors fade out. This
    problem could be due to the 68k being clocked at 11.8mhz when the hardware
    has a 16mhz crystal on it. Various timing loops show 11.8 being the average
    speed of the cpu and this does run true when comparing emulation and real
    hardware when timing is not based on Vsync (ssf2 and ssf2t for example). It is
    possible that what is slowing the cpu is read/write wait states when accessing
    RAM areas. This would mean that in places where lots of opcodes are being used
    in connection with data registers only the code would end up running to slow.

  - Giga Wing's sprites are 1 frame out when compared to background scrolling. See
    the explanation above for the most likely cause of this problem.

  - Progear slows down more than it should when compared to real hardware. See
    the explanation above for the most likely cause of this problem.

  - Some Hispanic/Brazil region sets have settings adjustable for a card dispenser.
    Many times, this is defaulted to ON.  Since MAME at this time does not emulate
    this unique dispenser, you will get a "NO CARD" message flashing on the screen
    for these sets unless you enter Service Mode and adjust CONFIGURATION > SYSTEM >
    C. DISPENSER to OFF.  An example of a game which does this is Street Fighter 3
    Alpha.


Driver Note:

  - Any new region sets will need full encryption tables dumped to extract the proper
    keys or they will need to be brute forced. XORs are no longer supported nor wanted.


Stephh's inputs notes (based on some tests on the "parent" set) :

0) All games

  - All inputs have been mapped according to the "test mode"
    (even if some buttons don't physically exist on the machine).
  - Joysticks and buttons can be fully tested.
  - COINn sometimes don't show anything but are OK ingame.
  - Unless I notify something below for some games, there is no extra button !

1) 'mmancp2u'

  - BUTTON3 doesn't physically exist on the machine and has no effect ingame.

2) 'megaman2'

  - BUTTON3 doesn't physically exist on the machine and has no effect ingame.

3) 'mpang'

  - BUTTON2 doesn't physically exist on the machine and can't be seen in the "test mode".
    However, if you map it where it should be, it has the same effect as BUTTON1.

4) 'pzloop2'

  - Whatever your settings are, the paddle can't be tested in the "test mode" !
    I can't tell at the moment if it's an emulation or an ingame bug :(

5) 'dimahoo'

  - BUTTON3 doesn't physically exist on the machine.
    However, it acts like a rapid fire (keep button pressed).

6) 'progear'

  - BUTTON3 acts like a rapid fire (keep button pressed).
    It has to be enabled in the game settings as it is OFF by default.

***************************************************************************/

#include "emu.h"

#include "cps1.h"
#include "cps2comm.h"
#include "cps2crypt.h"

#include "cpu/z80/z80.h"
#include "machine/eepromser.h"
#include "machine/nvram.h"

#include "qsoundx.h"
#include "speaker.h"

namespace {

/*************************************
 *
 *  Constants
 *
 *************************************/

// Maximum size of QSound Z80 region
#define QSOUND_SIZE 0x50000

// Maximum 68000 code size
#undef  CODE_SIZE
#define CODE_SIZE   0x0400000


class cps2x_state : public cps_state
{
public:
	cps2x_state(const machine_config &mconfig, device_type type, const char *tag)
		: cps_state(mconfig, type, tag)
		, m_objram(*this, "objram%u", 1U)
		, m_output(*this, "output")
		, m_decrypted_opcodes(*this, "decrypted_opcodes")
		, m_region_key(*this, "key")
		, m_comm(*this, "comm")
		, m_digitalvol(*this, "DIGITALVOL")
		, m_vs4p_playmode(*this, "PLAYMODE")
		, m_paddle(*this, "PADDLE%u", 1U)
		, m_qsoundx(*this, "qsound")
	{
		// Every cps2x machine carries qsoundx at the stock tag; the inherited stock-HLE finder
		// would only find a device of the wrong type, so it looks nowhere.
		m_qsound.set_tag(*this, finder_base::DUMMY_TAG);
	}

	void cps2(machine_config &config) ATTR_COLD;
	void cps2_4p_43(machine_config &config) ATTR_COLD; // 4-player, STOCK 4:3 (2v2 variants; NO widescreen)
	void cps2_4p_duo(machine_config &config) ATTR_COLD; // 4-player 4-live DUO + widescreen (mvscduo standalone)
	void cps2x(machine_config &config) ATTR_COLD; // CPS-2X expanded board (mvscextra): stock game + expansion chips

	void init_cps2() ATTR_COLD;
	void init_cps2nc() ATTR_COLD;
	// 4-player 2v2 tag mod: one init per set, supplying that set's on-point gate addresses.
	void init_vs4p(uint32_t gate1, uint32_t gate2) ATTR_COLD;
	void init_xmvsf_4p() ATTR_COLD;
	void init_mshvsf_4p() ATTR_COLD;
	void init_mvsc2v2_4p() ATTR_COLD;
	void init_mvscduo() ATTR_COLD;
	void init_mvscextra() ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void video_start() override ATTR_COLD;

private:
	static constexpr unsigned CPS2_OBJ_BASE  = 0x00 / 2;    // Unknown (not base address of objects). Could be base address of bank used when object swap bit set?
	static constexpr unsigned CPS2_OBJ_UNK1  = 0x02 / 2;    // Unknown (nearly always 0x807d, or 0x808e when screen flipped)
	static constexpr unsigned CPS2_OBJ_PRI   = 0x04 / 2;    // Layer priorities
	static constexpr unsigned CPS2_OBJ_UNK2  = 0x06 / 2;    // Unknown (usually 0x0000, 0x1101 in ssf2, 0x0001 in 19XX)
	static constexpr unsigned CPS2_OBJ_XOFFS = 0x08 / 2;    // X offset (usually 0x0040)
	static constexpr unsigned CPS2_OBJ_YOFFS = 0x0a / 2;    // Y offset (always 0x0010)

	void init_digital_volume();
	void cps2_eeprom_port_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint16_t cps2_qsound_volume_r();
	TIMER_CALLBACK_MEMBER(cps2_update_digital_volume);

	void cps2_objram_bank_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	template <unsigned Xor> uint16_t cps2_objram_r(offs_t offset);
	template <unsigned Xor> void cps2_objram_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

	void unshuffle(uint64_t *buf, int len);
	void cps2_gfx_decode();
	virtual void find_last_sprite() override;
	void cps2_render_sprites(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect, uint32_t *primasks);
	void cps2_objram_latch(int state);
	uint16_t *cps2_objbase();
	virtual void render_layers(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect) override;

	void cps2_base_map(address_map &map) ATTR_COLD;
	void cps2_map(address_map &map) ATTR_COLD;
	void decrypted_opcodes_map(address_map &map) ATTR_COLD;

	// CPS-2X expanded board (mvscextra). See the block comment above cps2x_map.
	void cps2x_map(address_map &map) ATTR_COLD;
	void cps2x_opcodes_map(address_map &map) ATTR_COLD;
	// Object tile codes gain bit 18 from object-record y bit 15 and bit 19 from x bit 10, and
	// object x bits 11-12 select the extended sprite palette pages 4-5. Set by init_mvscextra
	// only; stock sets never see the extra terms.
	bool m_cps2x_ext_obj = false;
	// CPS-2X sound extension. See the block comment above cps2x_qsound_sub_map.
	void cps2x_qsound_sub_map(address_map &map) ATTR_COLD;
	// cps_state::qsound_sub_map with the chip handlers on qsoundx.
	void qsoundx_sub_map(address_map &map) ATTR_COLD;
	void cps2x_qsound_banksw_w(uint8_t data);

	// 4-player 2v2 tag mod (VS trilogy). See the block comment above cps2_4p_in0_r.
	void cps2_4p_map(address_map &map) ATTR_COLD;
	bool vs4p_partner_on_point(int team);
	uint16_t vs4p_effective_playmode();
	uint16_t cps2_4p_in0_r();
	uint16_t cps2_4p_in1_r();
	uint16_t cps2_4p_in2_r();
	// Work-RAM address of each team's on-point index. 0 = not yet mapped for this set, which
	// leaves the mux inert and the game bit-for-bit stock.
	uint32_t m_vs4p_gate[2] = { 0, 0 };
	// CPS-2X: the Play Mode the GAME chose (its test menu's PLAYERS setting, written to 0x804056).
	uint16_t m_vs4p_playmode_game = 0;

	void init_cps2_video() ATTR_COLD;
	void init_cps2crypt() ATTR_COLD;

	required_shared_ptr_array<uint16_t, 2> m_objram;
	required_shared_ptr<uint16_t> m_output;

	optional_shared_ptr<uint16_t> m_decrypted_opcodes;
	optional_memory_region m_region_key;

	optional_device<cps2_comm_device> m_comm;

	optional_ioport m_digitalvol;
	// 1v1/2v2 selector. Present only on sets using cps2_4p6b_mode / cps2_4p6b_mode2v2. Absent
	// everywhere else (including mvscduo), where read_safe(0) yields 1v1 and the mux stays inert.
	optional_ioport m_vs4p_playmode;
	optional_ioport_array<2> m_paddle;
	optional_device<qsoundx_device> m_qsoundx; // the sound chip at :qsound (see qsoundx.h)

	// video-related
	std::unique_ptr<uint16_t[]> m_cps2_buffered_obj;
	int32_t m_cps2_last_sprite_offset = 0; // Offset of the last sprite
	int32_t m_pri_ctrl = 0; // Sprite layer priorities
	int32_t m_objram_bank = 0;
	int32_t m_cps2_obj_size = 0;

	// misc
	int32_t m_readpaddle = 0; // pzloop2
	int32_t m_cps2digitalvolumelevel = 0;
	int32_t m_cps2disabledigitalvolume = 0;
	emu_timer *m_digital_volume_timer = nullptr;
	int32_t m_cps2_dial_type = 0;
};


/*************************************
 *
 *  Video
 *
 *************************************/


void cps2x_state::unshuffle(uint64_t *buf, int len)
{
	if (len == 2)
		return;

	assert(len % 4 == 0);   /* must not happen */

	len /= 2;

	unshuffle(buf, len);
	unshuffle(buf + len, len);

	for (int i = 0; i < len / 2; i++)
	{
		const uint64_t t = buf[len / 2 + i];
		buf[len / 2 + i] = buf[len + i];
		buf[len + i] = t;
	}
}


void cps2x_state::cps2_gfx_decode()
{
	const int banksize = 0x200000;
	const auto size = memregion("gfx")->bytes();

	for (int i = 0; i < size; i += banksize)
		unshuffle((uint64_t *)(memregion("gfx")->base() + i), banksize / 8);
}


void cps2x_state::video_start()
{
	cps_state::video_start();

	m_cps2_obj_size = 0x2000;
	m_cps2_buffered_obj = make_unique_clear<uint16_t[]>(m_cps2_obj_size / 2);

	memset(m_objram[0], 0, m_cps2_obj_size);
	memset(m_objram[1], 0, m_cps2_obj_size);

	save_item(NAME(m_cps2_last_sprite_offset));
	save_pointer(NAME(m_cps2_buffered_obj), m_cps2_obj_size / 2);
	save_item(NAME(m_pri_ctrl));
	save_item(NAME(m_objram_bank));
}


void cps2x_state::cps2_objram_bank_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	if (ACCESSING_BITS_0_7)
		m_objram_bank = BIT(data, 0);
}

template <unsigned Xor>
uint16_t cps2x_state::cps2_objram_r(offs_t offset)
{
	return m_objram[m_objram_bank ^ Xor][offset];
}

template <unsigned Xor>
void cps2x_state::cps2_objram_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_objram[m_objram_bank ^ Xor][offset]);
}

uint16_t *cps2x_state::cps2_objbase()
{
	int baseptr = 0x7000;

	if (BIT(m_objram_bank, 0))
		baseptr ^= 0x0080;

	//popmessage("%04x %d", cps2_port(machine, CPS2_OBJ_BASE), BIT(m_objram_bank, 0));

	if (baseptr == 0x7000)
		return m_objram[0];
	else //if (baseptr == 0x7080)
		return m_objram[1];
}


void cps2x_state::find_last_sprite()    /* Find the offset of last sprite */
{
	cps_state::find_last_sprite();

	int offset = 0;
	uint16_t const *const base = m_cps2_buffered_obj.get();

	/* Locate the end of table marker */
	while (offset < m_cps2_obj_size / 2)
	{
		// CPS-2X (mvscextra): object y bit15 doubles as tile-code bit 18, so "y >= 0x8000"
		// would treat every bank-4+ sprite piece as the end-of-list marker (symptom: the
		// transplanted character is invisible and anything sorted after him vanishes). The
		// game's real marker is a record of all-0x8000 words, so require x AND y to match
		// exactly. Stock sets keep the original permissive test.
		// Object x bit10 is tile-code bit 19 on this board (banks 8-11, donor #2); a record
		// carrying it has x = 0x0400 | ..., which can never equal 0x8000, so the strict test
		// needs no further term -- a bank-8+ piece is never mistaken for the marker. The same holds
		// for the extended-palette bits x[11:12].
		const bool marker = m_cps2x_ext_obj
				? ((base[offset] == 0x8000 && base[offset + 1] == 0x8000) || base[offset + 3] >= 0xff00)
				: (base[offset + 1] >= 0x8000 || base[offset + 3] >= 0xff00);
		if (marker)
		{
			/* Marker found. This is the last sprite. */
			m_cps2_last_sprite_offset = offset - 4;
			return;
		}

		offset += 4;
	}

	/* Sprites must use full sprite RAM */
	m_cps2_last_sprite_offset = m_cps2_obj_size / 2 - 4;
}

void cps2x_state::cps2_render_sprites(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect, uint32_t *primasks)
{
#define DRAWSPRITE(CODE,COLOR,FLIPX,FLIPY,SX,SY)                                    \
{                                                                                   \
	if (flip_screen())                                                           \
		m_gfxdecode->gfx(2)->prio_transpen(bitmap,\
				cliprect,                                            \
				CODE,                                                               \
				COLOR,                                                              \
				!(FLIPX),!(FLIPY),                                                  \
				512-16-(SX),256-16-(SY), screen.priority(),primasks[priority],15);                 \
	else                                                                            \
		m_gfxdecode->gfx(2)->prio_transpen(bitmap,\
				cliprect,                                            \
				CODE,                                                               \
				COLOR,                                                              \
				FLIPX,FLIPY,                                                        \
				SX,SY, screen.priority(),primasks[priority],15);                 \
}

	uint16_t const *const base = m_cps2_buffered_obj.get();
	const int xoffs = 64 - m_output[CPS2_OBJ_XOFFS];
	const int yoffs = 16 - m_output[CPS2_OBJ_YOFFS];

	// Widescreen support (mvscduo): the visible window starts at raster 0, so sprite pieces
	// straddling the LEFT screen edge must draw at negative X. Plain `& 0x3ff` maps -16..-1 to
	// 0x3F0..0x3FF (far off the right side), which visibly crops fighters 16px per piece at the
	// widescreen left edge. Re-interpret that top 16px band as negative instead. Harmless for the
	// stock 4:3 sets: for them both -16..-1 and the 0..63 margin are outside the visible window.
	auto wrapx = [](int v) { v &= 0x3ff; return (v >= 0x3f0) ? v - 0x400 : v; };

	for (int i = m_cps2_last_sprite_offset; i >= 0; i -= 4)
	{
		int x = base[i + 0];
		int y = base[i + 1];
		const int priority = (x >> 13) & 0x07;
		// CPS-2X (mvscextra): object-record y bit 15 extends the tile code to bit 18 (donor #1
		// as banks 4-7) and object-record x bit 10 extends it to bit 19 (donor #2 as banks
		// 8-11); both set together reach banks 12-15, the mshx tile space of the 128MB region.
		// x[10:12] is unused by the stock builder -- x[0:9] is the position and
		// x[13:15] the priority -- and it is masked OUT of the position here so the piece lands
		// where the game put it. Stock boards leave the flag false and behave bit-identically.
		const int code = base[i + 2] + ((y & 0x6000) << 3)
				+ (m_cps2x_ext_obj ? (((y & 0x8000) << 3) | ((x & 0x0400) << 9)) : 0);
		// CPS-2X extended sprite palette: object x[11] moves the piece off palette page 0 onto
		// page 4 (colour codes 0x80-0x9F) and x[11]+x[12] onto page 5 (0xA0-0xBF). Those pages
		// are already copied from gfxram every time the palette base register is written
		// (control bits 4-5, see cps1_build_palette) and no CPS2 layer draws from them, so each
		// palette block (live 0x914000, freeze 0x92E000, ...) carries its own 64 extra sprite
		// lines at base+0x1000. x[12] without x[11] is reserved and selects page 0. With x[11]
		// clear the colour code is the stock `colour & 0x1f` unchanged. x[11:12] are masked out
		// of the position with x[10].
		const int ext_pal = m_cps2x_ext_obj ? ((x >> 11) & 0x03) : 0;
		if (m_cps2x_ext_obj)
			x &= ~0x1c00;
		const int colour = base[i + 3];
		const int col = (colour & 0x1f) + (BIT(ext_pal, 0) ? (BIT(ext_pal, 1) ? 0xa0 : 0x80) : 0);
		const bool flipx = BIT(colour, 5);
		const bool flipy = BIT(colour, 6);

		if (BIT(colour, 7))
		{
			x += m_output[CPS2_OBJ_XOFFS]; /* fix the offset of some games */
			y += m_output[CPS2_OBJ_YOFFS]; /* like Marvel vs. Capcom ending credits */
		}

		if (colour & 0xff00)
		{
			/* handle blocked sprites */
			int nx = (colour & 0x0f00) >> 8;
			int ny = (colour & 0xf000) >> 12;
			nx++;
			ny++;

			if (flipy)
			{
				/* Y flip */
				if (flipx)
				{
					for (int nys = 0; nys < ny; nys++)
					{
						const int sy = (y + nys * 16 + yoffs) & 0x3ff;
						for (int nxs = 0; nxs < nx; nxs++)
						{
							const int sx = wrapx(x + nxs * 16 + xoffs);
							DRAWSPRITE(
									code + (nx - 1) - nxs + 0x10 * (ny - 1 - nys),
									col,
									1, 1,
									sx, sy);
						}
					}
				}
				else
				{
					for (int nys = 0; nys < ny; nys++)
					{
						const int sy = (y + nys * 16 + yoffs) & 0x3ff;
						for (int nxs = 0; nxs < nx; nxs++)
						{
							const int sx = wrapx(x + nxs * 16 + xoffs);
							DRAWSPRITE(
									code + nxs + 0x10 * (ny - 1 - nys),
									col,
									0, 1,
									sx, sy);
						}
					}
				}
			}
			else
			{
				if (flipx)
				{
					for (int nys = 0; nys < ny; nys++)
					{
						const int sy = (y + nys * 16 + yoffs) & 0x3ff;
						for (int nxs = 0; nxs < nx; nxs++)
						{
							const int sx = wrapx(x + nxs * 16 + xoffs);
							DRAWSPRITE(
									code + (nx - 1) - nxs + 0x10 * nys,
									col,
									1, 0,
									sx, sy);
						}
					}
				}
				else
				{
					for (int nys = 0; nys < ny; nys++)
					{
						const int sy = (y + nys * 16 + yoffs) & 0x3ff;
						for (int nxs = 0; nxs < nx; nxs++)
						{
							const int sx = wrapx(x + nxs * 16 + xoffs);
							DRAWSPRITE(
									//code + nxs + 0x10 * nys,
									(code & ~0xf) + ((code + nxs) & 0xf) + 0x10 * nys, // pgear fix, same as CPS1?
									col,
									0, 0,
									sx, sy);
						}
					}
				}
			}
		}
		else
		{
			/* Simple case... 1 sprite */
			DRAWSPRITE(
					code,
					col,
					flipx, flipy,
					wrapx(x + xoffs), (y + yoffs) & 0x3ff);
		}
	}
#undef DRAWSPRITE
}


void cps2x_state::render_layers(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	/* Draw layers (0 = sprites, 1-3 = tilemaps) */
	const uint16_t layercontrol = m_cps_b_regs[m_game_config->layer_control / 2];
	uint8_t l0 = (layercontrol >> 0x06) & 0x03;
	uint8_t l1 = (layercontrol >> 0x08) & 0x03;
	uint8_t l2 = (layercontrol >> 0x0a) & 0x03;
	uint8_t l3 = (layercontrol >> 0x0c) & 0x03;
	screen.priority().fill(0, cliprect);

	uint32_t primasks[8]{};
	uint8_t l0pri = (m_pri_ctrl >> 4 * l0) & 0x0f;
	uint8_t l1pri = (m_pri_ctrl >> 4 * l1) & 0x0f;
	uint8_t l2pri = (m_pri_ctrl >> 4 * l2) & 0x0f;
	uint8_t l3pri = (m_pri_ctrl >> 4 * l3) & 0x0f;

#if 0
	if ((m_output[CPS2_OBJ_BASE] != 0x7080 && m_output[CPS2_OBJ_BASE] != 0x7000) ||
			m_output[CPS2_OBJ_UNK1] != 0x807d ||
			(m_output[CPS2_OBJ_UNK2] != 0x0000 && m_output[CPS2_OBJ_UNK2] != 0x1101 && m_output[CPS2_OBJ_UNK2] != 0x0001))
	{
		popmessage("base %04x uk1 %04x uk2 %04x",
				m_output[CPS2_OBJ_BASE],
				m_output[CPS2_OBJ_UNK1],
				m_output[CPS2_OBJ_UNK2]);
	}

	if (0 && machine().input().code_pressed(KEYCODE_Z))
		popmessage("order: %d (%d) %d (%d) %d (%d) %d (%d)",l0,l0pri,l1,l1pri,l2,l2pri,l3,l3pri);
#endif

	/* take out the CPS1 sprites layer */
	if (l0 == 0) { l0 = l1; l1 = 0; l0pri = l1pri; }
	if (l1 == 0) { l1 = l2; l2 = 0; l1pri = l2pri; }
	if (l2 == 0) { l2 = l3; l3 = 0; l2pri = l3pri; }

	{
		uint32_t mask0 = 0xaa;
		uint32_t mask1 = 0xcc;
		if (l0pri > l1pri) mask0 &= ~0x88;
		if (l0pri > l2pri) mask0 &= ~0xa0;
		if (l1pri > l2pri) mask1 &= ~0xc0;

		primasks[0] = 0xff;
		for (int i = 1; i < 8; i++)
		{
			if (i <= l0pri && i <= l1pri && i <= l2pri)
			{
				primasks[i] = 0xfe;
				continue;
			}
			primasks[i] = 0;
			if (i <= l0pri) primasks[i] |= mask0;
			if (i <= l1pri) primasks[i] |= mask1;
			if (i <= l2pri) primasks[i] |= 0xf0;
		}
	}

	cps1_render_layer(screen, bitmap, cliprect, l0, 1);
	cps1_render_layer(screen, bitmap, cliprect, l1, 2);
	cps1_render_layer(screen, bitmap, cliprect, l2, 4);
	cps2_render_sprites(screen, bitmap, cliprect, primasks);
}


void cps2x_state::cps2_objram_latch(int state)
{
	if (state)
	{
		m_pri_ctrl = m_output[CPS2_OBJ_PRI];
		memcpy(m_cps2_buffered_obj.get(), cps2_objbase(), m_cps2_obj_size);
	}
}


/*************************************
 *
 *  EEPROM
 *
 *************************************/

void cps2x_state::cps2_eeprom_port_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	if (ACCESSING_BITS_8_15)
	{
		/* bit 0 - Unused */
		/* bit 1 - Unused */
		/* bit 2 - Unused */
		/* bit 3 - Unused? */
		/* bit 4 - Eeprom data  */
		/* bit 5 - Eeprom clock */
		/* bit 6 - */
		/* bit 7 - */

		// EEPROM
		m_eepromout->write(data, 0xffff);
		if (m_cps2_dial_type == 2) // ecofghtr
		{
			m_readpaddle = BIT(data, 8);
		}
	}

	if (ACCESSING_BITS_0_7)
	{
		/* bit 0 - coin counter 1 */
		/* bit 1 - coin counter 2 */
		/* bit 2 - Unused */
		/* bit 3 - Allows access to Z80 address space (Z80 reset) */
		/* bit 4 - lock 1  */
		/* bit 5 - lock 2  */
		/* bit 6 - */
		/* bit 7 - */

		// Z80 Reset
		if (m_audiocpu != nullptr)
			m_audiocpu->set_input_line(INPUT_LINE_RESET, BIT(data, 3) ? CLEAR_LINE : ASSERT_LINE);

		machine().bookkeeping().coin_counter_w(0, BIT(data, 0));
		if (m_cps2_dial_type == 1) // pzloop2
		{
			// Puzz Loop 2 uses coin counter 2 input to switch between stick and paddle controls
			m_readpaddle = BIT(data, 1);
		}
		else
		{
			machine().bookkeeping().coin_counter_w(1, BIT(data, 1));
		}

		if (strncmp(machine().system().name, "mmatrix", 7) == 0) // Mars Matrix seems to require the coin lockout bit to be reversed
		{
			machine().bookkeeping().coin_lockout_w(0, BIT(data, 4));
			machine().bookkeeping().coin_lockout_w(1, BIT(data, 5));
			machine().bookkeeping().coin_lockout_w(2, BIT(data, 6));
			machine().bookkeeping().coin_lockout_w(3, BIT(data, 7));
		}
		else
		{
			machine().bookkeeping().coin_lockout_w(0, BIT(~data, 4));
			machine().bookkeeping().coin_lockout_w(1, BIT(~data, 5));
			machine().bookkeeping().coin_lockout_w(2, BIT(~data, 6));
			machine().bookkeeping().coin_lockout_w(3, BIT(~data, 7));
		}

		/*
		output().set_led_value(0, data & 0x01);
		output().set_led_value(1, data & 0x10);
		output().set_led_value(2, data & 0x20);
		*/
	}
}


/*************************************
 *
 *  Sound ?
 *
 *************************************/

TIMER_CALLBACK_MEMBER(cps2x_state::cps2_update_digital_volume)
{
	const int vol_button_state = m_digitalvol->read();

	if (BIT(vol_button_state, 0)) m_cps2digitalvolumelevel -= 1;
	if (BIT(vol_button_state, 1)) m_cps2digitalvolumelevel += 1;

	if (m_cps2digitalvolumelevel > 39) m_cps2digitalvolumelevel = 39;
	if (m_cps2digitalvolumelevel < 0) m_cps2digitalvolumelevel = 0;

	m_qsoundx->set_output_gain(0, m_cps2digitalvolumelevel / 39.0);
	m_qsoundx->set_output_gain(1, m_cps2digitalvolumelevel / 39.0);
}

uint16_t cps2x_state::cps2_qsound_volume_r()
{
	static const uint16_t cps2_vol_states[40] =
	{
		0xf010, 0xf008, 0xf004, 0xf002, 0xf001, 0xe810, 0xe808, 0xe804, 0xe802, 0xe801,
		0xe410, 0xe408, 0xe404, 0xe402, 0xe401, 0xe210, 0xe208, 0xe204, 0xe202, 0xe201,
		0xe110, 0xe108, 0xe104, 0xe102, 0xe101, 0xe090, 0xe088, 0xe084, 0xe082, 0xe081,
		0xe050, 0xe048, 0xe044, 0xe042, 0xe041, 0xe030, 0xe028, 0xe024, 0xe022, 0xe021
	};

	const uint16_t result = cps2_vol_states[m_cps2digitalvolumelevel];

	// Extra adapter memory (0x660000-0x663fff) available when bit 14 = 0
	// Network adapter (ssf2tb) present when bit 15 = 0
	// Only game known to use both these so far is SSF2TB

	if (m_comm && m_comm->comm_enabled())
		return 0x2021; // SSF2TB doesn't have a digital slider in the test screen
	else if (m_cps2disabledigitalvolume)
		return 0xd000; // Digital display isn't shown in test mode
	else
		return result;
}


/*************************************
 *
 *  Memory map
 *
 *************************************/

void cps2x_state::cps2_base_map(address_map &map)
{
	map(0x000000, 0x3fffff).rom();                                                                                                    // 68000 ROM
	map(0x400000, 0x40000b).ram().share(m_output);                                                                                    // CPS2 object output
	map(0x618000, 0x619fff).rw(FUNC(cps2x_state::qsound_sharedram_r<0>), FUNC(cps2x_state::qsound_sharedram_w<0>));                         // Q RAM
	map(0x660000, 0x663fff).ram();                                                                                                    // When bit 14 of 0x804030 equals 0 this space is available. Many games store highscores and other info here if available.
	map(0x664000, 0x664001).ram();                                                                                                    // Unknown - Only used if 0x660000-0x663fff available (could be RAM enable?)
	map(0x700000, 0x701fff).w(FUNC(cps2x_state::cps2_objram_w<0>)).share(m_objram[0]);                                                     // Object RAM, no game seems to use it directly
	map(0x708000, 0x709fff).mirror(0x006000).rw(FUNC(cps2x_state::cps2_objram_r<1>), FUNC(cps2x_state::cps2_objram_w<1>)).share(m_objram[1]); // Object RAM
	map(0x800100, 0x80013f).w(FUNC(cps2x_state::cps1_cps_a_w)).share(m_cps_a_regs);                                                    // Mirror (sfa)
	map(0x800140, 0x80017f).rw(FUNC(cps2x_state::cps1_cps_b_r), FUNC(cps2x_state::cps1_cps_b_w)).share(m_cps_b_regs);                   // Mirror (sfa)
	map(0x804000, 0x804001).portr("IN0");                                                                                             // IN0
	map(0x804010, 0x804011).portr("IN1");                                                                                             // IN1
	map(0x804020, 0x804021).portr("IN2");                                                                                             // IN2 + EEPROM
	map(0x804030, 0x804031).r(FUNC(cps2x_state::cps2_qsound_volume_r));                                                                // Master volume. Also when bit 14=0 addon memory is present, when bit 15=0 network adapter present.
	map(0x804040, 0x804041).w(FUNC(cps2x_state::cps2_eeprom_port_w));                                                                  // EEPROM
	map(0x8040a0, 0x8040a1).nopw();                                                                                                   // Unknown (reset once on startup)
	map(0x8040b0, 0x8040b2).lr8(NAME([this](offs_t offset) { return m_dsw[offset]->read(); }));                                       // DIP switches (only present on development hardware)
	map(0x8040e0, 0x8040e1).w(FUNC(cps2x_state::cps2_objram_bank_w));                                                                  // bit 0 = Object ram bank swap
	map(0x804100, 0x80413f).w(FUNC(cps2x_state::cps1_cps_a_w)).share(m_cps_a_regs);                                                    // CPS-A custom
	map(0x804140, 0x80417f).rw(FUNC(cps2x_state::cps1_cps_b_r), FUNC(cps2x_state::cps1_cps_b_w));                                       // CPS-B custom
	map(0x900000, 0x92ffff).ram().w(FUNC(cps2x_state::cps1_gfxram_w)).share(m_gfxram);                                                 // Video RAM
}

void cps2x_state::cps2_map(address_map &map)
{
	cps2_base_map(map);
	map(0xff0000, 0xffffff).ram();                                                                                                    // RAM
}

// --- CPS-2X expanded board (mvscextra) ---
// A fictional expanded B-board for the roster-expansion project: the stock CPS2 map plus TWO
// ROM "chips" mapped into the big unmapped hole at 0x930000-0xFEFFFF (~6.9MB). Both sit
// OUTSIDE the encryption range, so their contents are plaintext -- and both are mapped into
// the opcodes space below, so 68k code placed there executes as-is with no encode step.
// Capcom's own map reserved 0x660000-0x663FFF as "addon memory"; this extends the same idea.
// Stock sets never reference these addresses, so the mapping is inert for them.
//
// WHOLE-IMAGE MOUNTING (2026-08-17).  The layout changed from one packed chip to a split:
//
//   0x930000-0xD2FFFF  "donor"     4MB  a DONOR GAME'S WHOLE PROGRAM IMAGE, verbatim
//   0xD30000-0xE2FFFF  "expansion" 1MB  the host-side build products (blit lane)
//
// The donor chip carries mshvsf's entire 4MB program ROM, loaded with the SAME
// ROM_LOAD16_WORD_SWAP the maincpu region uses, so its byte order is normalised to the
// 68k's big-endian view exactly like ':maincpu' (a raw ROM_REGION would have held A^1
// pair-swapped bytes -- that asymmetry was live-debug bug 1 and it dies here).  Every
// address inside the donor image therefore translates into mvscextra's map by ONE
// CONSTANT, +0x930000: donor address A is readable at A + 0x930000, and every sub-block,
// chain, variant record and stream the donor ever had is present by construction.
// Rounds 9-21 of the transplant campaign were a running battle with INCOMPLETE EXTRACTION
// (carry-map sub-blocks left behind, variant records clamped, streams truncated); mounting
// the image whole makes that entire bug class structurally impossible.
//
// The "expansion" chip keeps the old job at a new base: rebuilt charID master tables,
// trampoline glue, lifted 68k code and synthesized stubs -- everything that does not exist
// in either original ROM.  It is still ERASEFF and still filled at runtime by the loader
// probe.  1MB against a payload measured in the low hundreds of KB.
//
// DONOR #2 (xmvsf, 2026-09-02) is a SPLIT MOUNT of one 3.5MB chip.  xmvsf's program image
// is seven 0x80000 files = 0x380000, which no single free run on the map holds: the hole
// left above the expansion chip is 1.75MB and the largest other run, 0x40000C-0x617FFF, is
// ~2.09MB.  So the ':donor2' region is mapped as two windows, piecewise constant:
//
//   0xE30000-0xFEFFFF  "donor2" segment 1  donor2 0x000000-0x1BFFFF  (+0xE30000, exact fit)
//   0x410000-0x5CFFFF  "donor2" segment 2  donor2 0x1C0000-0x37FFFF  (+0x250000)
//
// Both windows are in program AND opcodes space, plaintext, ROM_REGION16_BE +
// ROM_LOAD16_WORD_SWAP like ':donor', so donor #2 address A reads at A + 0xE30000 when
// A < 0x1C0000 and at A + 0x250000 otherwise. Segment 2 sits 64KB above the 12-byte object
// output block at 0x400000 and ends 0x48000 below the Q RAM at 0x618000; nothing else on
// this board's map (no comm device on cps2x) lives in between.
//
// THE MSHX CHIP (2026-09-25) is a BUILT chip, not a mount: the converter emits the msh
// characters already translated into mvsc's dialect (VS group ids, 16-byte records, mvsc
// struct offsets), so it follows the ':expansion' precedent -- plaintext, in program AND
// opcodes space, ERASEFF until the loader probe blits `mshx.bin` -- and NOT the donor
// precedent of a whole verbatim image:
//
//   0x710000-0x7FFFFF  "mshx"   960KB  the converted msh characters (code + data)
//   0x620000-0x65FFFF  "mshx2"  256KB  growth window, empty until a build fills it
//
// 0x710000 is the first byte above the object-RAM mirror (0x708000 + mirror 0x6000 ends at
// 0x70FFFF) and 0x7FFFFF the last below the CPS-A register mirror at 0x800100, so ':mshx'
// is the whole hole. The growth window sits above the Q RAM (0x618000-0x619FFF) and below
// the add-on RAM at 0x660000; the comm-device registers at 0x620000-0x620021 exist only on
// cps2_comm_map, which this board does not use. A decoded-instruction census of mvsc,
// mshvsf and xmvsf found no address-shaped reference into either window (one `ori.l
// #$7f0018` data constant each), so both are inert for the host game and for the donors.
// Unlike ':expansion' both are ROM_REGION16_BE, like the donor chips, so the region's byte
// order matches the space and a blit lands verbatim in the 68k view (no A^1 pair swap).
//
// THE TELEMETRY NVRAM (2026-10-23): 16KB of battery-backed work RAM at 0x600000-0x603FFF, in
// the free run between donor #2's second window (ends 0x5CFFFF) and the Q RAM (0x618000). The
// game's test menu keeps its character / support usage counters there (7 CONFIGURATION >
// 4 TELEMETRY, tools/excise/rebaselib/telemetry.py); MAME saves it as nvram/mvscextra/telemetry
// and reloads it at boot, like the 93C46's own file. The stock EEPROM path is untouched. A
// decoded-instruction census of the three images finds no memory operand in the run (two
// `#$600000` data constants, not addresses), so the region is inert for stock code.
//
// THE CONVERSATION CHIP (2026-10-24): 608KB of plaintext ROM at 0x668000-0x6FFFFF, ROM_REGION16_BE
// like the mshx chips and mapped in program AND opcodes space. It holds the match-start
// conversation lane (tools/excise/rebaselib/uiconvo.py): its sequencer, the 64x64 pair table and
// every page of dialog the editor's slot files carry. The lane grows with the roster (one script
// per character pair), so it gets a chip of its own instead of a corner of ':mshx'. The run
// 0x664002-0x6FFFFF is unmapped on this board (the add-on RAM ends at 0x663FFF, its enable word
// at 0x664001; the object RAM starts at 0x700000). A decoded-instruction census of the three
// images finds eight hits in the run: immediates (data constants) and one odd-address long read
// from a misdecoded data block, so nothing the programs execute reaches it. ERASEFF until the
// loader probe blits `ui_convo.bin`; the baked set ROM_LOADs `mvx_convo.bin`.
//
// THE STATUS CHIP (2026-10-24): 12KB of plaintext ROM at 0x665000-0x667FFF, ROM_REGION16_BE like the
// conversation chip and mapped in program AND opcodes space. It holds the STATUS lane
// (tools/excise/rebaselib/uistatus.py): the boot screen's text (the game name, the build date, the
// region line) and the two changelog pages shown in the WARNING and TRADEMARKS screens' slots, with
// the three small routines that draw them. It sits in the same unmapped run as the conversation chip,
// directly below it; 0x664002-0x664FFF stays unmapped above the add-on RAM's enable word. The census
// of 0x664002-0x6FFFFF quoted above covers it. ERASEFF until the loader probe blits `ui_status.bin`;
// the baked set ROM_LOADs `mvx_status.bin`.
void cps2x_state::cps2x_map(address_map &map)
{
	// Built on the 4-player map: the CPS-2X board carries the P3/P4 mod, with the "Play Mode"
	// selector (cps2_4p6b_mode) choosing stock 1v1/2P (mux inert) or 2v2/4P at the input layer.
	cps2_4p_map(map);
	// The EFFECTIVE Play Mode, readable by the program at 0x804054: bit 0 is the bit the 2v2 mux
	// tests (0 = 1v1, 1 = 2v2), the other bits read 0. The game WRITES its own choice (the test
	// menu's PLAYERS setting, kept in its EEPROM) to 0x804056; the MAME "Play Mode" setting is an
	// OVERRIDE for testing: "From game settings" (the default) returns the game's choice, "1v1" /
	// "2v2" force it. Both sit next to the raw P3/P4 pads at 0x804050/0x804052 in the unmapped gap
	// of the CPS2 I/O map. Only the CPS-2X board maps them: the ROM selector reads 0x804054 to
	// decide whether a team's second pick belongs to P3/P4, and the input mux honors the same
	// value. The stock 4-player sets keep cps2_4p_map as it is (their programs never read them).
	map(0x804054, 0x804055).lr16(NAME([this]() -> uint16_t { return vs4p_effective_playmode(); }));
	map(0x804056, 0x804057).lw16(NAME([this](offs_t offset, uint16_t data, uint16_t mem_mask) { if (ACCESSING_BITS_0_7) m_vs4p_playmode_game = data & 0x0001; }));
	map(0x410000, 0x5cffff).rom().region("donor2", 0x1c0000);                                                                         // CPS-2X donor #2 chip, segment 2: donor2 0x1C0000-0x37FFFF
	map(0x600000, 0x603fff).ram().share("telemetry");                                                                                 // CPS-2X telemetry NVRAM: 16KB battery-backed work RAM (see the comment above)
	map(0x620000, 0x65ffff).rom().region("mshx2", 0);                                                                                 // CPS-2X mshx growth chip: empty until a build fills it
	map(0x665000, 0x667fff).rom().region("status", 0);                                                                                // CPS-2X status chip: the boot screen and the two changelog pages (see the comment above)
	map(0x668000, 0x6fffff).rom().region("convo", 0);                                                                                 // CPS-2X conversation chip: the match-start dialog lane (see the comment above)
	map(0x710000, 0x7fffff).rom().region("mshx", 0);                                                                                  // CPS-2X mshx chip: the converted msh characters (built, plaintext)
	map(0x930000, 0xd2ffff).rom().region("donor", 0);                                                                                 // CPS-2X donor chip: a whole donor program image, verbatim
	map(0xd30000, 0xe2ffff).rom().region("expansion", 0);                                                                             // CPS-2X expansion chip: host-side build products
	map(0xe30000, 0xfeffff).rom().region("donor2", 0);                                                                                // CPS-2X donor #2 chip, segment 1: donor2 0x000000-0x1BFFFF
}

// --- CPS-2X sound extension: the audio CPU's flat data space, widened ---
//
// The QSound Z80 addresses ALL of its data -- master sound-ID table, sample descriptors,
// instrument tables and note scripts -- through one FLAT space that it pages into the
// 0x8000-0xBFFF window itself:
//
//     bank = (flat >> 14) - 2 ;  D003 <- bank | 0x80 ;  addr = 0x8000 | (flat & 0x3FFF)
//
// `bank` is computed in the accumulator and is therefore already EIGHT BITS WIDE in the
// firmware, and `or $80` at the eight sites that write D003 is what makes 0x00-0x7F the
// distinguishable range. So the Z80 program has always been able to address flat
// 0x000000-0x207FFF = 2.03MB; what capped it at 288KB was the DRIVER -- `qsound_banksw_w`
// keeps only `data & 0x0f`, and QSOUND_SIZE reserves 16 banks.
//
// The CPS-2X board answers that the same way it answers the object and sample walls: the
// storage grows and the format does not change. ':audiocpu' becomes 0x210000 (128 banks,
// which IS the firmware's own ceiling), and this handler keeps seven bits instead of four.
// Stock sets are untouched -- they keep QSOUND_SIZE, the stock handler and 16 banks -- and
// this one is reached only from cps2x_qsound_sub_map.
//
// The overflow guard is the STOCK one, unchanged: a bank past the end of the region reads
// as bank 0 and logs, so a wrong page is loud rather than silent.
void cps2x_state::cps2x_qsound_banksw_w(uint8_t data)
{
	int bank = data & 0x7f;
	if ((0x10000 + (bank * 0x4000)) >= m_audioregion->bytes())
	{
		logerror("%s: WARNING: CPS-2X Q sound bank overflow (%02x)\n", machine().describe_context(), data);
		bank = 0;
	}

	m_audiobank->set_entry(bank);
}

// The stock sound map (cps_state::qsound_sub_map in cps1.cpp), with the chip on qsoundx.
void cps2x_state::qsoundx_sub_map(address_map &map)
{
	map(0x0000, 0x7fff).rom();
	map(0x8000, 0xbfff).bankr(m_audiobank);    /* banked (contains music data) */
	map(0xc000, 0xcfff).ram().share(m_qsound_sharedram[0]);
	map(0xd000, 0xd002).w(m_qsoundx, FUNC(qsoundx_device::qsound_w));
	map(0xd003, 0xd003).w(FUNC(cps2x_state::qsound_banksw_w));
	map(0xd007, 0xd007).r(m_qsoundx, FUNC(qsoundx_device::qsound_r));
	map(0xf000, 0xffff).ram().share(m_qsound_sharedram[1]);
}

void cps2x_state::cps2x_qsound_sub_map(address_map &map)
{
	qsoundx_sub_map(map);
	map(0xd003, 0xd003).w(FUNC(cps2x_state::cps2x_qsound_banksw_w));   // 7-bit bank instead of 4
}

// --- 4-player 2v2 tag mod (xmvsf) ---
// Teams P1+P3 vs P2+P4. Each team shows one "point" character at a time. Ownership follows the
// CHARACTER, not the slot: P1/P2 own the STARTING (first-selected) character; P3/P4 own the
// PARTNER (second-selected). So we route the partner's pad into the game's P1/P2 bit-field only
// while the partner's character is the active point fighter.
//
// ---------------------------------------------------------------------------------------------
// 4-player 2v2 tag mod (VS trilogy: xmvsf, mshvsf, mvsc). Teams are P1+P3 vs P2+P4.
//
// HOW THESE GAMES TAG: the engine moves the CHARACTER, not the input routing. Each side has one
// slot the game always drives, and tagging swaps character state into it -- so P1 always drives
// team 1's active fighter whoever that currently is, and P2 always drives team 2's. That is what
// makes a port-level mux viable at all: there is no per-character input pointer to chase, only
// one byte per team saying whether the partner is currently the one in.
//
// THE THREE TITLES DO NOT SHARE A LAYOUT -- only the engine skeleton. Do not port addresses
// between them without first confirming the structure is shared:
//   xmvsf : four fixed slots. Tag routine @0x010250 exchanges fields FF4000<->FF4800 (team 1)
//           and FF4400<->FF4C00 (team 2). On-point index = +0x220 of the active slot, reads 0/1.
//   mshvsf: an indexed ARRAY at FF3800, stride 0x400, slot index read from +0x94
//           (move.b $94(aN),d0; ror.w #6,d0; lea base -> base + idx*0x400). xmvsf's +0x220 field
//           does NOT exist here and its tag routine has no counterpart. Gate reads 3, not 1.
//   mvsc  : same indexed-array scheme as mshvsf, base FF3000. Because the indexing CODE was
//           confirmed identical first, mshvsf's gate offsets ported to it directly.
//
// THE MOD: when a team's partner is on point, substitute that team's second player's pad for
// the first player's bits in the stock port reads. The benched player's pad going dead is the
// intended behaviour -- control follows the character to its owner.
//
// P3/P4 pads live in their own ioports (IN_P3/IN_P4) and are ALSO memory-mapped at 0x804050/
// 0x804052 (an unmapped gap on CPS2). The mapping is not needed by the mux; it exists so the
// pads can be read directly -- from a Lua probe or the debugger -- to tell "P3 isn't wired up"
// apart from "the gate never fired", which is otherwise very hard to distinguish.
//
// WHY NOT A ROM PATCH: a driver change is not a preference, it is forced. CPS2 has three input
// registers and a 6-button game already spends them (IN0 = P1+P2 sticks + buttons 1-3, IN1 =
// their buttons 4-6, and P2's button 6 does not even fit -- it lives in IN2 at 0x4000).
// 4 players x (4 dirs + 6 buttons) = 40 bits; IN0+IN1 provide 32. Players 3 and 4 cannot exist
// on this board, so no program patch can add them -- only the driver can invent the inputs.
// Given that, the routing may as well live here too, which keeps the games' ROMs stock.
// NOTE: this mod therefore runs in MAME only; it cannot work on real CPS2 hardware.
//
// HISTORY (do not regress): an earlier revision gated on FF4444/FF4C44, believing xmvsf's
// fighter structs were team-major. They are not -- that map paired bytes from two DIFFERENT
// teams, so the "flag" appeared to churn at random and the mux flickered. The approach was
// sound; one address was wrong. Every gate here is verified live by a self-driving test
// (14 assertions per game: gate on/off, partner drives, benched pad dead, both directions).
//
// --- STAGE 2 (in progress on mvsc): all four fighters live at once -----------------------------
// Found 2026-07-16, Lua-prototyped, not yet in this driver. Recorded here because the eventual
// C++ lands in this file and the facts below are load-bearing.
//
// HOW MVSC ROUTES INPUT: one reader routine (0x1367E) runs per fighter per frame. It picks the
// fighter's input word through a pointer table @0x136F4 indexed by struct byte +0x02 ("which pad
// drives me": 0 -> FF444C, 1 -> FF444E); +0x03 selects human path vs CPU/AI path. Cur/prev/edge
// input words are per-fighter IN THE STRUCT (+0xD0..+0xDA). In 2P play all four fighter slots
// take the human path with pad indices 0/1/0/1 -- a Duo partner reads its team's word, which is
// the entire "both characters move as one" problem.
//
// THE PLANNED PATCH (all inside m_decrypted_opcodes; ROMs stay stock, dipswitch-gateable):
//   * stub at 0x3FF000 (dead 0xFF fill) carrying its OWN 4-entry pointer table:
//     pads 0/1 -> FF444C/FF444E; pads 2/3 -> this driver's raw P3/P4 ports at 804050/804052
//     plus a not.w (ports are active-low; the caller's andi #$77f masks).
//   * 0x13698: movea.l $136f4(pc,d0.w),a0 + move.w (a0),d0 (6 bytes) -> jsr $3ff000.l (6 bytes).
//   * giving a fighter to P3/P4 is then ONE RAM BYTE: FF3802=2 / FF3C02=3.
//
// !! ENCRYPTION RULE (a v1 stub crashed on this): the 68000 routes PC-RELATIVE operand reads
// through the OPCODE space, so Capcom stores data tables INSIDE the encrypted region and reads
// them (pc)-relative -- the table @136F4 is ciphertext to a normal (An) data read. Any relocated
// pc-relative code must keep such reads pc-relative, or embed its data with the code in the
// decrypted buffer (the stub embeds). Symptom otherwise: garbage pointer -> address error the
// first time a human fighter polls input (attract fighters are CPU-driven and never hit it).
//
// MVSC 4-LIVE COSMETIC STATE (for the C++ port of the mode; all user-verified in Lua):
//   * 1P/2P OVER-HEAD marker arrows (Duo-Mode only): permanent pool objects in FF8000-FFDFxx
//     with handler ptr +0x34 in {0x86EF0, 0x86F08}; visibility = object +0x04. FLICKER-FREE
//     removal (user-verified) = a WRITE-TAP on each marker's +0x04 that forces 0 (returns 0),
//     so the enable value never lands -> the arrow never draws. Rescan the pool periodically
//     to tap respawns. Do NOT touch the OFF-SCREEN LOCATOR arrow (bottom-of-screen, points to
//     an out-of-view opponent on super-jump) -- it is a DIFFERENT handler and is desirable.
//     Driver port: space.install_write_tap() on the marker +0x04, same as the Lua reference.
//     (A per-frame zero of +0x04 also works but flickers on the KO re-enable; use the tap.)
//   * duo bg effect: video staging FF443E committed to reg 0x804166, gated by FF4445/FF4439 --
//     clear the GATES to restore the stage backdrop (writing the staged VALUE blanks the layer).
//   * duo timer bar: starves when the singleton duo timers FF4034/FF4036 are zero.
//   * top-HUD (health bar art) kill switch if ever needed: beq->bra at 0x249DA skips the
//     staged-sprite copy from GFX-RAM 0x924000 (count word at FFF640).
//   * KO JUMP-IN CUT (user-verified): on KO the surviving partner (already on the field)
//     plays the stock entry jump-in. Trigger is NOT FF4022 (status word), the +0x26A "enter
//     field" command byte, or the 0x7B934 controller spawn -- all tested dead ends. Real
//     path (found live: bp 0x15EE4 entry-teleport + history): dying fighter gets command 4
//     at +0x266 -> dispatcher 0xD6F6 -> 0x15B76 (death bookkeeping) -> 0x15BA8 `movea.l
//     $284(a6),a6` switches to the partner and runs its entry init (teleport to camX+0x210
//     via 0x15EBE, entry velocity). THE CUT: 0x15BA8 `2C6E 0284` -> `6000 0106` (bra.w
//     $15CB0) skips ONLY the partner-entry block, keeping all death bookkeeping and the
//     partner-dead branch 0x15CB4. The engine's own point-status transfer then does the
//     right thing (survivor inherits point + camera focus). Decrypted-buffer patch.
//   * WIDESCREEN WALLS (goes with the 512px set_raw in cps2_4p below; user-verified): fighter
//     X is in STAGE coordinates; the screen-edge clamp is camera-relative, in the common
//     movement routine at 0x13AE2: a0 = ptrtable[0x2563A + stage*4] -> struct whose word +0
//     is the camera X; left wall = camX + imm@0x13AF6 (stock 0x55), right wall = camX +
//     imm@0x13B0A (stock 0x1AB) -- both 0x15 (21px) inside the stock 384px edges. For the
//     full 512px view patch the two immediates (decrypted buffer) to 0x15 / 0x1EB: same
//     margins, walkable 342px -> 470px. Struct byte +0xB9 = touching-wall flag (2=L, 1=R).
//   * WIDESCREEN CAMERA CLAMP (user-verified; goes with the 512px view): the 512px window
//     over-scrolls past the tilemap at stage ends because the game's camera-scroll clamp is
//     tuned for the 384px window. The clamp is at 0xCCB68: camera obj (a6) target +0x76 is
//     limited between +0x68 (min bound, = -0x40 at stage left) and +0x6A (max bound, per
//     stage), result -> +0xC, then staged to FF443A (scroll1 X) by 0xCB94A. Bounds are
//     register-loaded (no immediate to patch) and the camera obj address is DYNAMIC, so the
//     fix is a CODE-CAVE: hijack 0xCCB68 with `jsr <cave>` + `bra $ccb86`; the cave re-runs
//     the clamp with min +0x40 / max -0x40 (pull each scroll limit in 64px = the 512-384
//     extra-width, 64/side). Kills the corner void. NOTE (still open): after this, the
//     PARALLAX SCROLL PLANES still render within 4:3 -- the per-plane draw/scroll width is
//     tied to 384px; widening the visible raster (set_raw) did not widen the plane draw
//     window. That is a separate driver/video fix (scroll-plane render width vs 512 raster).
//   * SCREEN-ANCHORED SPECIALS (user-verified both facings): a handful of moves anchor to
//     the screen edge via the idiom `d0 = camX (same 0x2563A struct); addi.w #$40 (left
//     edge); if facing (+0x4B==0 means facing left): addi.w #$180 (right edge)`. Exactly 7
//     sites ROM-wide; Jin's Blodia Punch owns 4 (calcs 0x9F05A/0x9F230/0x9E10A + the
//     BINDING travel cap, a cmpi/move #$150 pair on ext +0xC2 at 0x9F14C/0x9F154; the
//     0x160/0x80 caps beside the calcs never bind). Widescreen: 0x40->0x00, 0x180->0x200,
//     cap 0x150->0x1D0 (tip = edge+0x30, stock proportion). A full-ROM idiom scan found only
//     8 edge sites; the other CAPPED screen-filler is a SHARED screen-super routine at
//     0x79DE0 (anchor +0xC0, ext +0xC2, cap 0x160): patch it the same way (0x40->0x00,
//     0x180->0x200, cap 0x160->0x200) and ONE patch widens Captain Commando's Captain Sword,
//     Gambit's Royal Flush, War Machine's Proton Cannon, etc. (user-verified 2026-07-17).
//     The rest are movement (dash-to-edge velocity 0x38934/0x38F9C/0x92136, facing +0xB3,
//     no cap) + Jin teleport 0x9E54E -- NOT screen-fillers, leave them. Fighter-relative
//     moves and travel-till-offscreen projectiles need NO changes. Arm objects: script ptr
//     +0x68 (character-script pointer, not 68k code), X +0x0C, owner +0x54, ext +0xC2.
//
// !! PAD-BYTE RULE (found via live debugger; a v2 stub broke the hit system on this): mvsc's
// collision-side registration (0x4AB4) does `tst.b ($2,a6)` -- fighter struct +0x02 (the pad
// index) doubles as the TEAM SIDE: zero = side A, nonzero = side B. Giving partners pad
// indices 2/3 puts both on side B -> most attack pairs stop interacting and P1 gains friendly
// fire. Therefore +0x02 must stay 0/1 FOREVER. STUB v3 (user-verified in Lua, probe47) routes
// input by SLOT ADDRESS instead: the reader stub compares a6 against 0xFF3800/0xFF3C00 and
// serves the raw P3/P4 ports (804050/804052, not.w for polarity) for those fighters, the
// original word table for everyone else. Control then follows the CHARACTER (P3 always drives
// the team's second pick, through tags and duos) -- for mvsc this also supersedes the port mux
// below, which the C++ port of the 4-live mode should disable/bypass on that set.
// The Play Mode the board acts on. The "Play Mode" ioport (cps2_4p6b_mode / _mode2v2) is 1 bit
// on the stock 4-player sets (0 = 1v1, 1 = 2v2). On the CPS-2X set it is 2 bits: 0 = force 1v1,
// 1 = force 2v2, 2 = "From game settings" = whatever the game last wrote to 0x804056 (the default
// there; a cfg saved with the old 1-bit field keeps its meaning: 0 = 1v1, 1 = 2v2). Absent
// everywhere else (mvscduo): 1v1, the mux inert.
uint16_t cps2x_state::vs4p_effective_playmode()
{
	const uint16_t sw = m_vs4p_playmode.read_safe(0) & 0x0003;
	if (sw == 2)
		return m_vs4p_playmode_game & 0x0001;
	return sw & 0x0001;
}

bool cps2x_state::vs4p_partner_on_point(int team)
{
	// "Play Mode" = 1v1, or no selector on this set (mvscduo) -> P1/P2 pass through untouched.
	// Same inert path as an unmapped gate, so a 1v1 launch is bit-for-bit the stock game.
	if (!(vs4p_effective_playmode() & 0x01))
		return false;
	if (!m_vs4p_gate[team])
		return false;                       // set not mapped yet -> stay stock
	return m_maincpu->space(AS_PROGRAM).read_byte(m_vs4p_gate[team]) != 0;
}

// IN0: P1 = low byte, P2 = high byte (4 dirs + buttons 1-3 each).
// IN_P3/IN_P4 mirror the game's own 11-bit word layout, so their low byte matches IN0's.
uint16_t cps2x_state::cps2_4p_in0_r()
{
	uint16_t r = ioport("IN0")->read();
	if (vs4p_partner_on_point(0))
		r = (r & 0xff00) | (ioport("IN_P3")->read() & 0x00ff);          // P3 takes over P1's bits
	if (vs4p_partner_on_point(1))
		r = (r & 0x00ff) | ((ioport("IN_P4")->read() & 0x00ff) << 8);   // P4 takes over P2's bits
	return r;
}

// IN1: P1 buttons 4-6 = bits 0-2, P2 buttons 4-5 = bits 4-5 (P2's button 6 did not fit here --
// it lives in IN2, see below). In IN_P3/IN_P4, buttons 4-6 are bits 8-10.
uint16_t cps2x_state::cps2_4p_in1_r()
{
	uint16_t r = ioport("IN1")->read();
	if (vs4p_partner_on_point(0))
		r = (r & 0xfff8) | ((ioport("IN_P3")->read() >> 8) & 0x0007);   // P3 buttons 4-6
	if (vs4p_partner_on_point(1))
		r = (r & 0xffcf) | ((ioport("IN_P4")->read() >> 4) & 0x0030);   // P4 buttons 4-5
	return r;
}

// IN2 carries P2's button 6 at 0x4000 (alongside EEPROM/coins/starts/service) because IN1 ran
// out of room in the 6-button layout. P4 needs it too, or it could not complete FP+FK to tag.
uint16_t cps2x_state::cps2_4p_in2_r()
{
	uint16_t r = ioport("IN2")->read();
	if (vs4p_partner_on_point(1))
	{
		const uint16_t b6 = (ioport("IN_P4")->read() >> 10) & 1;        // active-low: 0 = pressed
		r = (r & 0xbfff) | (b6 << 14);
	}
	return r;
}

void cps2x_state::cps2_4p_map(address_map &map)
{
	cps2_map(map);
	map(0x804000, 0x804001).r(FUNC(cps2x_state::cps2_4p_in0_r));
	map(0x804010, 0x804011).r(FUNC(cps2x_state::cps2_4p_in1_r));
	map(0x804020, 0x804021).r(FUNC(cps2x_state::cps2_4p_in2_r));
	map(0x804050, 0x804051).portr("IN_P3");   // raw pads, for probes/debugger (see block comment)
	map(0x804052, 0x804053).portr("IN_P4");
}

void cps2x_state::decrypted_opcodes_map(address_map &map)
{
	map(0x000000, 0x3fffff).rom().share(m_decrypted_opcodes); // 68000 ROM
}

void cps2x_state::cps2x_opcodes_map(address_map &map)
{
	decrypted_opcodes_map(map);
	map(0x410000, 0x5cffff).rom().region("donor2", 0x1c0000); // CPS-2X donor #2 chip, segment 2: executable in place, unencrypted
	map(0x620000, 0x65ffff).rom().region("mshx2", 0);         // CPS-2X mshx growth chip: executable, unencrypted
	map(0x665000, 0x667fff).rom().region("status", 0);        // CPS-2X status chip: executable, unencrypted
	map(0x668000, 0x6fffff).rom().region("convo", 0);         // CPS-2X conversation chip: executable, unencrypted
	map(0x710000, 0x7fffff).rom().region("mshx", 0);          // CPS-2X mshx chip: executable, unencrypted
	map(0x930000, 0xd2ffff).rom().region("donor", 0);         // CPS-2X donor chip: executable in place, unencrypted
	map(0xd30000, 0xe2ffff).rom().region("expansion", 0);     // CPS-2X expansion chip: executable, unencrypted
	map(0xe30000, 0xfeffff).rom().region("donor2", 0);        // CPS-2X donor #2 chip, segment 1: executable in place, unencrypted
}


/*************************************
 *
 *  Generic port definitions
 *
 *************************************/

// 4 players and 4 buttons
static INPUT_PORTS_START( cps2_4p4b )
	PORT_START("IN0")      // (0x00)
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)

	PORT_START("IN1")      // (0x10)
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)

	PORT_START("IN2")      // (0x20)
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_READ_LINE_DEVICE_MEMBER("eeprom", FUNC(eeprom_serial_93cxx_device::do_read))
	PORT_SERVICE_NO_TOGGLE( 0x0002, IP_ACTIVE_LOW )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN4 )

	PORT_START( "EEPROMOUT" )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_OUTPUT ) PORT_WRITE_LINE_DEVICE_MEMBER("eeprom", FUNC(eeprom_serial_93cxx_device::di_write))
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_OUTPUT ) PORT_WRITE_LINE_DEVICE_MEMBER("eeprom", FUNC(eeprom_serial_93cxx_device::clk_write))
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_OUTPUT ) PORT_WRITE_LINE_DEVICE_MEMBER("eeprom", FUNC(eeprom_serial_93cxx_device::cs_write))

	// Fake inputs for digital volume buttons
	PORT_START( "DIGITALVOL" )
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_VOLUME_DOWN )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_VOLUME_UP )

	// Machine configuration for dev hardware with DIP switches
	PORT_START( "HW_TYPE" )
	PORT_CONFNAME( 0x01, 0x00, "Hardware" )
	PORT_CONFSETTING(    0x00, "Production" )
	PORT_CONFSETTING(    0x01, "Development (with debug DIP switches)" )

	PORT_START("DSWA")
	PORT_DIPNAME( 0x01, 0x01, "1-1" ) PORT_DIPLOCATION("SW1:1") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "1-2" ) PORT_DIPLOCATION("SW1:2") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "1-3" ) PORT_DIPLOCATION("SW1:3") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "1-4" ) PORT_DIPLOCATION("SW1:4") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "1-5" ) PORT_DIPLOCATION("SW1:5") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "1-6" ) PORT_DIPLOCATION("SW1:6") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "1-7" ) PORT_DIPLOCATION("SW1:7") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "1-8" ) PORT_DIPLOCATION("SW1:8") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x00)

	PORT_START("DSWB")
	PORT_DIPNAME( 0x01, 0x01, "2-1" ) PORT_DIPLOCATION("SW2:1") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "2-2" ) PORT_DIPLOCATION("SW2:2") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "2-3" ) PORT_DIPLOCATION("SW2:3") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "2-4" ) PORT_DIPLOCATION("SW2:4") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "2-5" ) PORT_DIPLOCATION("SW2:5") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "2-6" ) PORT_DIPLOCATION("SW2:6") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "2-7" ) PORT_DIPLOCATION("SW2:7") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "2-8" ) PORT_DIPLOCATION("SW2:8") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x00)

	PORT_START("DSWC")
	PORT_DIPNAME( 0x01, 0x01, "3-1" ) PORT_DIPLOCATION("SW3:1") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "3-2" ) PORT_DIPLOCATION("SW3:2") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "3-3" ) PORT_DIPLOCATION("SW3:3") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "3-4" ) PORT_DIPLOCATION("SW3:4") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "3-5" ) PORT_DIPLOCATION("SW3:5") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "3-6" ) PORT_DIPLOCATION("SW3:6") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "3-7" ) PORT_DIPLOCATION("SW3:7") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "3-8" ) PORT_DIPLOCATION("SW3:8") PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x01)
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED ) PORT_CONDITION("HW_TYPE", 0x01, EQUALS, 0x00)
INPUT_PORTS_END

// 4 players and 3 buttons
// 4 players and 2 buttons
// 3 players and 4 buttons
static INPUT_PORTS_START( cps2_3p4b )
	PORT_INCLUDE(cps2_4p4b)

	PORT_MODIFY("IN1")
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED ) // PORT_PLAYER(4) inputs

	PORT_MODIFY("IN2")
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED ) // START4
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED ) // COIN4
INPUT_PORTS_END

// 3 players and 3 buttons
// 3 players and 2 buttons
#ifdef UNUSED_DEFINITION
#endif

/* 2 players and 4 buttons */
static INPUT_PORTS_START( cps2_2p4b )
	PORT_INCLUDE(cps2_3p4b)

	PORT_MODIFY("IN1")
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED ) // PORT_PLAYER(3) inputs

	PORT_MODIFY("IN2")
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED ) // START3
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED ) // COIN3
INPUT_PORTS_END

// 2 players and 3 buttons
static INPUT_PORTS_START( cps2_2p3b )
	PORT_INCLUDE(cps2_2p4b)

	PORT_MODIFY("IN0")
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED ) // BUTTON4 PORT_PLAYER(1)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED ) // BUTTON4 PORT_PLAYER(2)
INPUT_PORTS_END


// 2 players and 2 buttons
// 2 players and 1 button
// 2 players and 6 buttons (2 rows of 3 buttons)
static INPUT_PORTS_START( cps2_2p6b )
	PORT_INCLUDE(cps2_2p3b)

	PORT_MODIFY("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)

	PORT_MODIFY("IN2")
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)
INPUT_PORTS_END

// 4 players, 6 buttons — 2v2 tag mod (VS trilogy: xmvsf, mshvsf, mvsc).
// Teams are P1+P3 vs P2+P4; P3/P4 drive each team's SECOND (tagged-in) character.
//
// IN0/IN1/IN2 are left completely STOCK, so P1/P2 behave exactly as on unmodified hardware.
// P3/P4 instead get their own read-only registers (IN_P3 @0x804050, IN_P4 @0x804052) in the
// unmapped 0x804050-0x80409f gap of the CPS2 I/O map. A new register is required, not a
// preference: CPS2's native 4-player wiring puts P3/P4 in IN1, but a 6-button game already
// spends IN1 on buttons 4-6 (P2's button 6 doesn't even fit — it lives in IN2 at 0x4000).
// 4 players x (4 dirs + 6 buttons) = 40 bits > the 32 bits IN0+IN1 provide.
//
// The bit layout below deliberately MIRRORS the 11-bit word the game itself builds in work RAM
// (see the master input routine: word = (IN1_low << 8) | IN0_low, then NOT + AND #$7ff):
//   bit0 Right, 1 Left, 2 Down, 3 Up, 4 B1, 5 B2, 6 B3, (7 unused), 8 B4, 9 B5, 10 B6
// Active-low like every other CPS2 input. Mirroring the layout keeps the companion ROM patch
// down to: move.w <reg>,d0 / not.w d0 / andi.w #$7ff,d0 / move.w d0,<player word>.
static INPUT_PORTS_START( cps2_4p6b )
	PORT_INCLUDE(cps2_2p6b)

	PORT_START("IN_P3")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(3)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(3)
	PORT_BIT( 0xf800, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("IN_P4")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(4)
	PORT_BIT( 0xf800, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

// ---------------------------------------------------------------------------------------------
// 1v1 / 2v2 SELECTOR
//
// PORT_CONFNAME, not PORT_DIPNAME. This is read by the DRIVER (vs4p_partner_on_point), never by
// the game program: the VS titles have no concept of a third or fourth player, so an emulated PCB
// dipswitch would be inert -- there is no ROM code to branch on it. The entire 2v2 mod lives in
// the driver, so the switch belongs there too.
//
// Two variants, differing ONLY in default:
//   cps2_4p6b_mode     -> 1v1. For the STOCK Capcom sets, where an untouched launch must be
//                         indistinguishable from unmodified MAME. 2v2 is opt-in.
//   cps2_4p6b_mode2v2  -> 2v2. For the modded *2v2 sets, whose whole purpose is 4-player; they
//                         come up ready to play, with 1v1 as the fallback.
// ---------------------------------------------------------------------------------------------
// CPS-2X: the switch is an OVERRIDE over the game's own PLAYERS setting (test menu, EEPROM):
// "From game settings" is the default; 1v1 / 2v2 force it for testing.
static INPUT_PORTS_START( cps2x_mode )
	PORT_INCLUDE(cps2_4p6b)

	PORT_START("PLAYMODE")
	PORT_CONFNAME( 0x03, 0x02, "Play Mode" )
	PORT_CONFSETTING(    0x02, "From game settings (test menu PLAYERS)" )
	PORT_CONFSETTING(    0x00, "1v1 (override)" )
	PORT_CONFSETTING(    0x01, "2v2 (override)" )
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( cps2_4p6b_mode2v2 )
	PORT_INCLUDE(cps2_4p6b)

	PORT_START("PLAYMODE")
	PORT_CONFNAME( 0x01, 0x01, "Play Mode" )
	PORT_CONFSETTING(    0x00, "1v1 (stock, 2 players)" )
	PORT_CONFSETTING(    0x01, "2v2 (4 players)" )
	PORT_BIT( 0xfffe, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

// ---------------------------------------------------------------------------------------------
// mvscduo ports -- DELIBERATELY A STANDALONE COPY, NOT PORT_INCLUDE(cps2_4p6b).
//
// INSULATION. duo is 4-LIVE: all four fighters are on the field at once, its P3/P4 routing is
// baked into the program ROM (STUB v3 @0x13698 reads 804050/804052 directly), and init_mvscduo
// leaves m_vs4p_gate at {0,0} so the driver mux never runs. It therefore needs the P3/P4 ports
// but must NEVER inherit anything the 2v2 work adds -- a selector here would be a menu entry that
// does nothing, since duo has no 1v1 mode to switch to.
//
// An earlier attempt added the selector to the shared cps2_4p6b and it leaked straight into duo.
// Copying the port definitions makes that class of accident structurally impossible. The cost is
// ~24 duplicated lines; cps2_2p6b is still shared because that is genuine CPS2 base hardware, not
// part of this mod's surface.
//
// ⚠ STILL SHARED WITH THE 2v2 SETS: cps2_4p_map (creates 804050/804052). duo's program ROM
// HARDCODES those two addresses -- moving them silently breaks the baked stub. Change with care.
// ---------------------------------------------------------------------------------------------
static INPUT_PORTS_START( cps2_4p6b_duo )
	PORT_INCLUDE(cps2_2p6b)

	PORT_START("IN_P3")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(3)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(3)
	PORT_BIT( 0xf800, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("IN_P4")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(4)
	PORT_BIT( 0xf800, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

// 2 players, 6 buttons, and 1 ticket dispenser (2 rows of 3 buttons)
// 1 player and 4 buttons
// 1 player and 3 buttons
// 1 player and 2 buttons

/*************************************
 *
 *  Game-specific port definitions
 *
 *************************************/

// According to the "test mode", buttons layout look like a 2 players 6 buttons machine where buttons have been removed
// 2 players, no joysticks which are replaced with 4 buttons, no other buttons
// 2 players and 1 button - either 2 8-way joysticks, 2 2-way joysticks, or 2 paddles
// 1 player and 3 buttons, but 2 coins slots

/*************************************
 *
 *  Machine driver
 *
 *************************************/

void cps2x_state::machine_start()
{
	if (m_audiocpu != nullptr) // gigaman2 has an AT89C4051 (8051) MCU as an audio cpu, no qsound.
	{
		// Bank count follows the REGION, not the QSOUND_SIZE constant: every stock set declares
		// its ':audiocpu' region as QSOUND_SIZE, so this is the same 16 banks it always was, and
		// the CPS-2X board (which declares a larger region, see ROM_START(mvscextra)) gets the
		// banks its own region actually holds.
		memory_region *const rgn = memregion("audiocpu");
		m_audiobank->configure_entries(0, (rgn->bytes() - 0x10000) / 0x4000, rgn->base() + 0x10000, 0x4000);
	}
}


void cps2x_state::cps2(machine_config &config)
{
	// Basic machine hardware
	M68000(config, m_maincpu, 16_MHz_XTAL);
	m_maincpu->set_interrupt_mixer(false);
	m_maincpu->set_addrmap(AS_PROGRAM, &cps2x_state::cps2_map);
	m_maincpu->set_addrmap(AS_OPCODES, &cps2x_state::decrypted_opcodes_map);
	m_maincpu->set_addrmap(m68000_base_device::AS_CPU_SPACE, &cps2x_state::cpu_space_map);

	TIMER(config, "scantimer").configure_scanline(FUNC(cps2x_state::raster_scanline), "screen", 0, 1);

	Z80(config, m_audiocpu, 8_MHz_XTAL);
	m_audiocpu->set_addrmap(AS_PROGRAM, &cps2x_state::qsoundx_sub_map);

	const attotime audio_irq_period = attotime::from_hz(8_MHz_XTAL / 32000); // measured 250Hz
	m_audiocpu->set_periodic_int(FUNC(cps2x_state::irq0_line_hold), audio_irq_period);

	EEPROM_93C46_16BIT(config, "eeprom");

	// Video hardware
	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_raw(CPS_PIXEL_CLOCK, CPS_HTOTAL, CPS_HBEND, CPS_HBSTART, CPS_VTOTAL, CPS_VBEND, CPS_VBSTART);
	m_screen->set_screen_update(FUNC(cps2x_state::screen_update_cps1));
	m_screen->screen_vblank().set(FUNC(cps2x_state::screen_vblank_cps1));
	m_screen->screen_vblank().append(FUNC(cps2x_state::cps2_objram_latch));
	m_screen->set_palette(m_palette);

	GFXDECODE(config, m_gfxdecode, m_palette, gfx_cps1);
	PALETTE(config, m_palette, palette_device::BLACK).set_entries(0xc00);

	// Sound hardware
	SPEAKER(config, "speaker", 2).front();

	QSOUNDX(config, m_qsoundx); // the stock chip: 24-bit (16MB) sample space
	m_qsoundx->add_route(0, "speaker", 1.0, 0);
	m_qsoundx->add_route(1, "speaker", 1.0, 1);
}

// NOTE: the old shared `cps2_4p` config (4-player ports + a 512px widescreen raster) has been
// REMOVED. It was the widescreen feasibility experiment and its only remaining user was the STOCK
// mvsc parent, which had no business being widescreen or 4-player. The two survivors below are
// explicit about their screen: cps2_4p_43 (2v2 variants, stock 4:3) and cps2_4p_duo (mvscduo,
// widescreen). Per the project's hard build rule: WIDESCREEN = mvscduo ONLY.

void cps2x_state::cps2_4p_43(machine_config &config)
{
	cps2(config);
	// 4-player input (P3/P4 registers at 0x804050/0x804052) with the STOCK 4:3 screen. Used by
	// every 2v2-capable set (stock xmvsf/mshvsf/mvsc + mshvsf2v2/mvsc2v2) -- NO widescreen, which
	// is mvscduo-only and lives in cps2_4p_duo.
	m_maincpu->set_addrmap(AS_PROGRAM, &cps2x_state::cps2_4p_map);
}

void cps2x_state::cps2_4p_duo(machine_config &config)
{
	cps2(config);
	// mvscduo standalone: 4-player 4-LIVE Duo mode on the widescreen raster. Same P3/P4 input map
	// as the 2v2 sets, but presented full-512px / 16:9 so all four fighters can traverse the wider
	// field (walls + screen-anchored specials are widened in the program ROM). This is a dedicated
	// config (not the shared cps2_4p) so the parent mvsc can later be restored to stock without
	// disturbing mvscduo.
	m_maincpu->set_addrmap(AS_PROGRAM, &cps2x_state::cps2_4p_map);

	// Full 512px raster (cols 0..512 vs stock 64..448) -- widest the CPS-A/B composes.
	m_screen->set_raw(CPS_PIXEL_CLOCK, CPS_HTOTAL, 0, CPS_HTOTAL, CPS_VTOTAL, CPS_VBEND, CPS_VBSTART);
	// Present at 16:9 so pixels keep their stock shape: 512/384 x (4:3) = 16:9.
	m_screen->set_physical_aspect(16, 9);
}

void cps2x_state::cps2x(machine_config &config)
{
	cps2(config);
	// CPS-2X expanded board (mvscextra): STOCK game rules on the stock 4:3 raster, carrying the
	// 4-player mod with the "Play Mode" input selector -- 1v1 (mux inert, bit-stock 2P) or 2v2
	// (proven 4P tag mux, mvsc gates). The other hardware delta is storage: donor/expansion ROM
	// at 0x930000, the split donor #2 windows and the built mshx chips at 0x710000/0x620000
	// (program + opcodes space, plaintext, see cps2x_map), a 128MB gfx region (object
	// tile-code bits 18/19 via y[15]/x[10], see cps2_render_sprites and init_mvscextra), and
	// a 32MB QSound sample region.
	m_maincpu->set_addrmap(AS_PROGRAM, &cps2x_state::cps2x_map);
	m_maincpu->set_addrmap(AS_OPCODES, &cps2x_state::cps2x_opcodes_map);
	// The telemetry NVRAM behind the 0x600000 share (see cps2x_map): zero when no file exists.
	NVRAM(config, "telemetry", nvram_device::DEFAULT_ALL_0);
	// Sound storage grows the same way: a 2MB ':audiocpu' region reached through a 7-bit bank
	// register instead of a 4-bit one. See cps2x_qsound_banksw_w.
	m_audiocpu->set_addrmap(AS_PROGRAM, &cps2x_state::cps2x_qsound_sub_map);
	// And the sample chip: its ROM space widened to 25 bits, so the 32MB ':qsound' region
	// (donor #2's samples at 0x1000000+) is reachable through the firmware's sample-page byte.
	// qsoundx_device is a device_rom_interface<24>, so the DSP's (bank << 16 | addr) sample
	// address would otherwise be truncated to 24 bits: a 9th bank bit (banks 0x100+) folds back
	// onto bank 0x00+. The firmware's sample-page stub emits (0x80 | page) as the data word's
	// high byte (atlas/sound-capacity.md s4; the HLE keeps bits 8-14: `bank &= 0x7fff`).
	m_qsoundx->set_sample_space_bits(25); // 32MB: bank byte 0x00-0xFF at page 0, 0x00-0xFF at page 1
}

/*************************************
 *
 *  ROM definitions
 *
 *************************************/

/*************************************
 *
 *  ROM definitions
 *
 *************************************/

// xmvsf2v2: the 4-player 2v2 mod on xmvsf. The mod is DRIVER-ONLY (init_xmvsf_4p) on bit-for-bit
// stock ROMs, so the set is the unmodified xmvsf (Europe 961004) dumps: a clone with no files of its own.
ROM_START( xmvsf2v2 )
	ROM_REGION( CODE_SIZE, "maincpu", 0 ) // 68000 code
	ROM_LOAD16_WORD_SWAP( "xvse.03f", 0x000000, 0x80000, CRC(db06413f) SHA1(c6d8aa1e43fc541e5b4e938258f27ab9ee30ca33) )
	ROM_LOAD16_WORD_SWAP( "xvse.04f", 0x080000, 0x80000, CRC(ef015aef) SHA1(d3504cb8c38f720b1f4528157266db60c8c6c075) )
	ROM_LOAD16_WORD_SWAP( "xvs.05a",  0x100000, 0x80000, CRC(7db6025d) SHA1(2d74f48f83f45359bfaca28ab686625766af12ee) )
	ROM_LOAD16_WORD_SWAP( "xvs.06a",  0x180000, 0x80000, CRC(e8e2c75c) SHA1(929408cb5d98e95cec75ea58e4701b0cbdbcd016) )
	ROM_LOAD16_WORD_SWAP( "xvs.07",   0x200000, 0x80000, CRC(08f0abed) SHA1(ef16c376232dba63b0b9bc3aa0640f9001ccb68a) )
	ROM_LOAD16_WORD_SWAP( "xvs.08",   0x280000, 0x80000, CRC(81929675) SHA1(19cf7afbc1daaefec40195e40ba74970f3906a1c) )
	ROM_LOAD16_WORD_SWAP( "xvs.09",   0x300000, 0x80000, CRC(9641f36b) SHA1(dcba3482d1ba37ccfb30d402793ee063c6621aed) )

	ROM_REGION( 0x2000000, "gfx", 0 )
	ROM_LOAD64_WORD( "xvs.13m",   0x0000000, 0x400000, CRC(f6684efd) SHA1(c0a2f3a9e82ab8b084a500aec71ac633e947328c) )
	ROM_LOAD64_WORD( "xvs.15m",   0x0000002, 0x400000, CRC(29109221) SHA1(898b8f678fd03c462ce0d8eb7fb3441ef601085b) )
	ROM_LOAD64_WORD( "xvs.17m",   0x0000004, 0x400000, CRC(92db3474) SHA1(7b6f4c8ebfdac167b25f35029068b6253c141fe6) )
	ROM_LOAD64_WORD( "xvs.19m",   0x0000006, 0x400000, CRC(3733473c) SHA1(6579da7145c95b3ad00844a5fc8c2e22c23365e2) )
	ROM_LOAD64_WORD( "xvs.14m",   0x1000000, 0x400000, CRC(bcac2e41) SHA1(838ff24f7e8543a787a55a5d592c9517ce3b8b93) )
	ROM_LOAD64_WORD( "xvs.16m",   0x1000002, 0x400000, CRC(ea04a272) SHA1(cd7c79037b5b4a39bef5156433e984dc4dc2c081) )
	ROM_LOAD64_WORD( "xvs.18m",   0x1000004, 0x400000, CRC(b0def86a) SHA1(da3a6705ea7050fc5c2c10d33400ed67be9f455d) )
	ROM_LOAD64_WORD( "xvs.20m",   0x1000006, 0x400000, CRC(4b40ff9f) SHA1(9a981d442132efff09a27408d74646ba357c7357) )

	ROM_REGION( QSOUND_SIZE, "audiocpu", 0 ) // 64k for the audio CPU (+banks)
	ROM_LOAD( "xvs.01",   0x00000, 0x08000, CRC(3999e93a) SHA1(fefcff8a9a5c83df7655a16187cf9ba3e7efbb25) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xvs.02",   0x28000, 0x20000, CRC(101bdee9) SHA1(75920e88bf46fcd33a7957777a1d799818ffb0d6) )

	ROM_REGION( 0x400000, "qsound", 0 ) // QSound samples
	ROM_LOAD16_WORD_SWAP( "xvs.11m",   0x000000, 0x200000, CRC(9cadcdbc) SHA1(64d3bd53b04daec84c9af4aa3ff010867b3d306d) )
	ROM_LOAD16_WORD_SWAP( "xvs.12m",   0x200000, 0x200000, CRC(7b11e460) SHA1(a581c84acaaf0ce056841c15a6f36889e88be68d) )

	ROM_REGION( 0x20, "key", 0 )
	ROM_LOAD( "xmvsf.key",    0x000000, 0x000014, CRC(d5c07311) SHA1(1b401ffc241436c4869486c174774b67e3bf3df8) )
ROM_END

// 4-player 2v2 variant, built on the Japan 970625 (mshvsfj2) program for the full roster incl.
// Norimaro (native on JP; euro removed his select-cell). P3/P4 routing uses the fixed +0x00
// on-field gate (FF4000/FF4400) which is base-independent -- the old +0x79 turbo was NOT a
// euro/JP difference, it was the wrong (pose-coupled) offset on both.
ROM_START( mshvsf2v2 )
	ROM_REGION( CODE_SIZE, "maincpu", 0 ) // 68000 code
	// mvsj2v2.03g = JP mvsj.03g + select-hook (JSR to secret handlers in .10b), CPS2-encrypted.
	ROM_LOAD16_WORD_SWAP( "mvsj2v2.03g", 0x000000, 0x80000, CRC(634ff285) SHA1(9b146217b980a942b306f60b9ed8268db38a18cb) )
	ROM_LOAD16_WORD_SWAP( "mvsj.04g", 0x080000, 0x80000, CRC(c921825f) SHA1(471e44268cebba631b81f131bf31e27b8a28c548) )
	ROM_LOAD16_WORD_SWAP( "mvs.05a",  0x100000, 0x80000, CRC(1a5de0cb) SHA1(738a27f83704c208d36d73bf766d861ef2d51a89) )
	ROM_LOAD16_WORD_SWAP( "mvs.06a",  0x180000, 0x80000, CRC(959f3030) SHA1(fbbaa915324815246738f3426232e623f039ce26) )
	ROM_LOAD16_WORD_SWAP( "mvs.07b",  0x200000, 0x80000, CRC(7f915bdb) SHA1(683da09c5ba55e31b59aa95a8e13c45dc574ab3c) )
	ROM_LOAD16_WORD_SWAP( "mvs.08a",  0x280000, 0x80000, CRC(c2813884) SHA1(49e5d4bc48f90c8146cb6aafb9240aff0119f1a7) )
	ROM_LOAD16_WORD_SWAP( "mvs.09b",  0x300000, 0x80000, CRC(3ba08818) SHA1(9ab132a3cac55fcccebe6c99b6fb0ba1305f8f6e) )
	// mvs2v2.10b = mvs.10b + the four secret-select handlers in its free space (plaintext, >enc range).
	ROM_LOAD16_WORD_SWAP( "mvs2v2.10b", 0x380000, 0x80000, CRC(807c5efc) SHA1(dbe09ed38b6dbf823786c597be774e809fd9095f) )

	ROM_REGION( 0x2000000, "gfx", 0 )
	ROM_LOAD64_WORD( "mvs.13m",   0x0000000, 0x400000, CRC(29b05fd9) SHA1(e8fdb1ee5515a560eb4256ae4fd99bb1192e1a87) )
	ROM_LOAD64_WORD( "mvs.15m",   0x0000002, 0x400000, CRC(faddccf1) SHA1(4ed03ea91883a0413325f57edcc1614120b5922c) )
	ROM_LOAD64_WORD( "mvs.17m",   0x0000004, 0x400000, CRC(97aaf4c7) SHA1(6a054921cc14fe080cb3f62c391f8ae3cc7e8ba9) )
	ROM_LOAD64_WORD( "mvs.19m",   0x0000006, 0x400000, CRC(cb70e915) SHA1(da4d2480d348ac6dfd01256a88f4f3db8357ae46) )
	ROM_LOAD64_WORD( "mvs.14m",   0x1000000, 0x400000, CRC(b3b1972d) SHA1(0f2c3fb7de014181ee481ec35d0578b2c116c2dc) )
	ROM_LOAD64_WORD( "mvs.16m",   0x1000002, 0x400000, CRC(08aadb5d) SHA1(3a2c222eca3e7df80ce69951b3db6442312751a4) )
	ROM_LOAD64_WORD( "mvs.18m",   0x1000004, 0x400000, CRC(c1228b35) SHA1(7afdfb552888c79d0fbb30242b3d917b87fad57a) )
	ROM_LOAD64_WORD( "mvs.20m",   0x1000006, 0x400000, CRC(366cc6c2) SHA1(6f2a789087c8e404c5227b927fa8328c03593243) )

	ROM_REGION( QSOUND_SIZE, "audiocpu", 0 ) // 64k for the audio CPU (+banks)
	ROM_LOAD( "mvs.01",   0x00000, 0x08000, CRC(68252324) SHA1(138ef320ef27956b2ab5591d49a1315b7b0a194c) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvs.02",   0x28000, 0x20000, CRC(b34e773d) SHA1(3bcf44bf06c35814cff29d244142db7abe05bd39) )

	ROM_REGION( 0x800000, "qsound", 0 ) // QSound samples
	ROM_LOAD16_WORD_SWAP( "mvs.11m",   0x000000, 0x400000, CRC(86219770) SHA1(4e5b68d382a5aa37f8b0b6434c53a2b95f5f9a4d) )
	ROM_LOAD16_WORD_SWAP( "mvs.12m",   0x400000, 0x400000, CRC(f2fd7f68) SHA1(28a30d55d3eaf963006c7cbe7c288099cd3ba536) )

	ROM_REGION( 0x20, "key", 0 )
	ROM_LOAD( "mshvsfj.key",  0x000000, 0x000014, CRC(565eeebb) SHA1(762844b59b2fcf529a26ad3dde8282415db926b3) )
ROM_END

// 4-player 2v2 variant on the USA (mvscu) base. mvcu2v2.03d/.04d = mvcu.03d/.04d + the "all
// characters" patch (sets the enable-all-chars flag 0x40 via a hooked routine), re-encrypted for
// mvscu.key. See tools/mvsc2v2_allchars.py. 4:3 (no widescreen); routing via +0x00 gate.
ROM_START( mvsc2v2 )
	ROM_REGION( CODE_SIZE, "maincpu", 0 ) // 68000 code
	ROM_LOAD16_WORD_SWAP( "mvcu2v2.03d", 0x000000, 0x80000, CRC(b1822dda) SHA1(12b2f3b7b2a45d86b952fc891b3ed1c853fe3513) )
	ROM_LOAD16_WORD_SWAP( "mvcu2v2.04d", 0x080000, 0x80000, CRC(dd2f1026) SHA1(6835390bc4b4c58d651f39c3ede543b31367d782) )
	ROM_LOAD16_WORD_SWAP( "mvc.05a",  0x100000, 0x80000, CRC(2d8c8e86) SHA1(b07d640a734c5d336054ed05195786224c9a6cd4) )
	ROM_LOAD16_WORD_SWAP( "mvc.06a",  0x180000, 0x80000, CRC(8528e1f5) SHA1(cd065c05268ab581b05676da544baf6af642acac) )
	ROM_LOAD16_WORD_SWAP( "mvc.07",   0x200000, 0x80000, CRC(c3baa32b) SHA1(d35589847e0753e869ffcd7c3abed925bfdb0fa2) )
	ROM_LOAD16_WORD_SWAP( "mvc.08",   0x280000, 0x80000, CRC(bc002fcd) SHA1(0b6735a071a9274f7ab25c743271fc30411fe819) )
	ROM_LOAD16_WORD_SWAP( "mvc.09",   0x300000, 0x80000, CRC(c67b26df) SHA1(6e9969246c57269d7ba0992a5cc319c8910bf8a9) )
	ROM_LOAD16_WORD_SWAP( "mvc.10",   0x380000, 0x80000, CRC(0fdd1e26) SHA1(5fa684d823b4f4eec61ed9e9b8938af5272ae1ed) )

	ROM_REGION( 0x2000000, "gfx", 0 )
	ROM_LOAD64_WORD( "mvc.13m",   0x0000000, 0x400000, CRC(fa5f74bc) SHA1(79a619248938a85ce4f7794a704647b9cf564fbc) )
	ROM_LOAD64_WORD( "mvc.15m",   0x0000002, 0x400000, CRC(71938a8f) SHA1(6982f7203458c1c46a1c1c13c0d0f2a5e109d271) )
	ROM_LOAD64_WORD( "mvc.17m",   0x0000004, 0x400000, CRC(92741d07) SHA1(ddfd70eab7c983ab452194b1860059f8ad694459) )
	ROM_LOAD64_WORD( "mvc.19m",   0x0000006, 0x400000, CRC(bcb72fc6) SHA1(46ab98dcdf6f5d611646a22a7355939ef5b2bbe5) )
	ROM_LOAD64_WORD( "mvc.14m",   0x1000000, 0x400000, CRC(7f1df4e4) SHA1(ede92b31c1fe87f91b4fe74ac211f2fb5f863bc2) )
	ROM_LOAD64_WORD( "mvc.16m",   0x1000002, 0x400000, CRC(90bd3203) SHA1(ed83208c486ea0f407b7e5d16a8cf242a6f73774) )
	ROM_LOAD64_WORD( "mvc.18m",   0x1000004, 0x400000, CRC(67aaf727) SHA1(e0e69104e31d2c41e18c0d24e9ab962406a7ca9a) )
	ROM_LOAD64_WORD( "mvc.20m",   0x1000006, 0x400000, CRC(8b0bade8) SHA1(c5732361bb4bf284c4d12a82ac2c5750b1f9d441) )

	ROM_REGION( QSOUND_SIZE, "audiocpu", 0 ) // 64k for the audio CPU (+banks)
	ROM_LOAD( "mvc.01",   0x00000, 0x08000, CRC(41629e95) SHA1(36925c05b5fdcbe43283a882d021e5360c947061) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvc.02",   0x28000, 0x20000, CRC(963abf6b) SHA1(6b784870e338701cefabbbe4669984b5c4e8a9a5) )

	ROM_REGION( 0x800000, "qsound", 0 ) // QSound samples
	ROM_LOAD16_WORD_SWAP( "mvc.11m",   0x000000, 0x400000, CRC(850fe663) SHA1(81e519d05a08855f242ea2e17ee0859b449db895) )
	ROM_LOAD16_WORD_SWAP( "mvc.12m",   0x400000, 0x400000, CRC(7ccb1896) SHA1(74caadf3282fcc6acffb1bbe3734106f81124121) )

	ROM_REGION( 0x20, "key", 0 )
	ROM_LOAD( "mvscu.key",    0x000000, 0x000014, CRC(a83db333) SHA1(7f7288ceadf233d913728f7c4a8841adcb5994e8) )
ROM_END

// mvscduo: mvsc EURO base with the widescreen 4-LIVE Duo patches baked in (tools/mvscduo_patch.py):
// STUB v3 input routing + hijack (.03a/.10), wide walls + KO cut + beam reach (.03a), Blodia reach
// (.04a). Encrypted-range words re-encrypted for mvsc.key; the stub body @0x3FF000 is plaintext in
// mvcduo.10. Shared euro data ROMs (mvc.05a-.09) + gfx/audio/qsound unchanged.
ROM_START( mvscduo )
	ROM_REGION( CODE_SIZE, "maincpu", 0 ) // 68000 code
	ROM_LOAD16_WORD_SWAP( "mvcduo.03a", 0x000000, 0x80000, CRC(7ffe4657) SHA1(802911b5cca8711de245fca296b40fd2e75f60c5) )
	ROM_LOAD16_WORD_SWAP( "mvcduo.04a", 0x080000, 0x80000, CRC(027d7306) SHA1(a7df8cae4ed49bf7aedab5c131bc271246dfa2c7) )
	ROM_LOAD16_WORD_SWAP( "mvc.05a",  0x100000, 0x80000, CRC(2d8c8e86) SHA1(b07d640a734c5d336054ed05195786224c9a6cd4) )
	ROM_LOAD16_WORD_SWAP( "mvc.06a",  0x180000, 0x80000, CRC(8528e1f5) SHA1(cd065c05268ab581b05676da544baf6af642acac) )
	ROM_LOAD16_WORD_SWAP( "mvc.07",   0x200000, 0x80000, CRC(c3baa32b) SHA1(d35589847e0753e869ffcd7c3abed925bfdb0fa2) )
	ROM_LOAD16_WORD_SWAP( "mvc.08",   0x280000, 0x80000, CRC(bc002fcd) SHA1(0b6735a071a9274f7ab25c743271fc30411fe819) )
	ROM_LOAD16_WORD_SWAP( "mvc.09",   0x300000, 0x80000, CRC(c67b26df) SHA1(6e9969246c57269d7ba0992a5cc319c8910bf8a9) )
	ROM_LOAD16_WORD_SWAP( "mvcduo.10", 0x380000, 0x80000, CRC(ae9075e8) SHA1(1aecd7f80c1562fa4200cc4068cb5cbaa3e08808) )

	ROM_REGION( 0x2000000, "gfx", 0 )
	ROM_LOAD64_WORD( "mvc.13m",   0x0000000, 0x400000, CRC(fa5f74bc) SHA1(79a619248938a85ce4f7794a704647b9cf564fbc) )
	ROM_LOAD64_WORD( "mvc.15m",   0x0000002, 0x400000, CRC(71938a8f) SHA1(6982f7203458c1c46a1c1c13c0d0f2a5e109d271) )
	ROM_LOAD64_WORD( "mvc.17m",   0x0000004, 0x400000, CRC(92741d07) SHA1(ddfd70eab7c983ab452194b1860059f8ad694459) )
	ROM_LOAD64_WORD( "mvc.19m",   0x0000006, 0x400000, CRC(bcb72fc6) SHA1(46ab98dcdf6f5d611646a22a7355939ef5b2bbe5) )
	ROM_LOAD64_WORD( "mvc.14m",   0x1000000, 0x400000, CRC(7f1df4e4) SHA1(ede92b31c1fe87f91b4fe74ac211f2fb5f863bc2) )
	ROM_LOAD64_WORD( "mvc.16m",   0x1000002, 0x400000, CRC(90bd3203) SHA1(ed83208c486ea0f407b7e5d16a8cf242a6f73774) )
	ROM_LOAD64_WORD( "mvc.18m",   0x1000004, 0x400000, CRC(67aaf727) SHA1(e0e69104e31d2c41e18c0d24e9ab962406a7ca9a) )
	ROM_LOAD64_WORD( "mvc.20m",   0x1000006, 0x400000, CRC(8b0bade8) SHA1(c5732361bb4bf284c4d12a82ac2c5750b1f9d441) )

	ROM_REGION( QSOUND_SIZE, "audiocpu", 0 ) // 64k for the audio CPU (+banks)
	ROM_LOAD( "mvc.01",   0x00000, 0x08000, CRC(41629e95) SHA1(36925c05b5fdcbe43283a882d021e5360c947061) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvc.02",   0x28000, 0x20000, CRC(963abf6b) SHA1(6b784870e338701cefabbbe4669984b5c4e8a9a5) )

	ROM_REGION( 0x800000, "qsound", 0 ) // QSound samples
	ROM_LOAD16_WORD_SWAP( "mvc.11m",   0x000000, 0x400000, CRC(850fe663) SHA1(81e519d05a08855f242ea2e17ee0859b449db895) )
	ROM_LOAD16_WORD_SWAP( "mvc.12m",   0x400000, 0x400000, CRC(7ccb1896) SHA1(74caadf3282fcc6acffb1bbe3734106f81124121) )

	ROM_REGION( 0x20, "key", 0 )
	ROM_LOAD( "mvsc.key",     0x000000, 0x000014, CRC(7e101e09) SHA1(9d725a7c6bbc20e46f749eaec4bab15b0195077a) )
ROM_END

// mvscextra: mvsc EURO base on the CPS-2X expanded board (roster-expansion project; see
// workshop-vs-characters.md in the project repo). The base game is BIT-STOCK (stock rules,
// stock 4:3, stock 2P at the "Play Mode" default) and the extra hardware is STORAGE.
//
// WHOLE-IMAGE MOUNTING (2026-08-17).  The donor game rides along IN ITS ENTIRETY, as three
// verbatim chip mounts, and the host game merely learns to reference it:
//
//   "donor"  4MB   mshvsf's whole 68k program image  -> 68k 0x930000-0xD2FFFF
//                  ROM_LOAD16_WORD_SWAP, i.e. the same normalisation ':maincpu' gets, so
//                  donor address A reads at A + 0x930000 with plain big-endian words.
//   "gfx"    upper 32MB   mshvsf's whole object ROM   -> tile codes 0x40000-0x7FFFF
//                  (CPS-2X banks 4-7, reached by object y[15] = tile-code bit 18).  Same
//                  8-chip ROM_LOAD64_WORD interleave as stock, so cps2_gfx_decode's
//                  per-0x200000-bank unshuffle covers it identically.  Donor tile c is
//                  host tile 0x40000 + c: donor gfx bank N lands on host bank 4+N, which
//                  is why the per-character +0x47 bank byte maps 0x00->0x10, 0x02->0x14.
//   "qsound" upper 8MB    mshvsf's whole sample ROM   -> sample banks 0x80-0xFF
//                  (bank b of the donor is bank b+0x80 here).
//
// DONOR #2 (xmvsf, 2026-09-02) rides along the same way, one size class up on every chip:
//
//   "donor2" 3.5MB xmvsf's whole 68k program image   -> 68k 0xE30000-0xFEFFFF (donor2
//                  0x000000-0x1BFFFF) + 0x410000-0x5CFFFF (donor2 0x1C0000-0x37FFFF);
//                  the image is 3.5MB and no single free run holds it, so it is one
//                  region mapped through two windows (see cps2x_map).
//   "gfx"    third 32MB   xmvsf's whole object ROM    -> tile codes 0x80000-0xBFFFF
//                  (CPS-2X banks 8-11, reached by object x[10] = tile-code bit 19).
//   "qsound" third 8MB    xmvsf's 4MB sample ROM      -> sample banks 0x100-0x13F
//                  (page 1 of the 32MB space; the cps2x config widens the qsoundx
//                  device's ROM space to 25 bits, and the firmware's sample-page byte
//                  selects the page -- atlas/sound-capacity.md s4).
//
// "expansion" (now 1MB at 0xD30000) stays ERASEFF and holds only what exists in NEITHER
// original ROM: rebuilt charID master tables, trampoline glue and lifted 68k code, blitted
// in at runtime by the loader probe.
//
// THE MSHX CHIPS (2026-09-25) carry the msh exclusives, and msh is NOT mounted: its
// characters are BUILT by the converter into mvsc's dialect, so what lands on the board is
// a build product like the expansion, never a verbatim msh image:
//
//   "mshx"   960KB  converted msh characters, code + data   -> 68k 0x710000-0x7FFFFF
//   "mshx2"  256KB  growth window, empty until a build fills it -> 68k 0x620000-0x65FFFF
//   "gfx"    fourth 32MB   msh tiles as CPS-2X banks 12-15  -> tile codes 0xC0000-0xFFFFF
//                  (reached by object y[15] AND x[10] both set: bits 18 and 19 together).
//   "qsound" unchanged: msh samples ride the existing page-1 banks 0x140-0x17F.
//
// Both mshx regions are ERASEFF (0xFFFF is an illegal 68k instruction, so a pointer aimed
// past a lane traps instead of executing zeros as `ori.b #0,d0`) and are filled at runtime
// by the loader probe during the build campaign; a release build gains ROM_LOAD lines.
//
// With "Play Mode" at 1v1 and no payload loaded the set still boots and plays identically
// to stock mvsc -- nothing in mvsc's program reaches any of the mounted address space.
ROM_START( mvscextra )
	ROM_REGION( CODE_SIZE, "maincpu", 0 ) // 68000 code
	ROM_LOAD16_WORD_SWAP( "mvce.03a", 0x000000, 0x80000, CRC(824e4a90) SHA1(5c79c166d988d8a75d9941f4ee6fa4d6476e55e1) )
	ROM_LOAD16_WORD_SWAP( "mvce.04a", 0x080000, 0x80000, CRC(436c5a4e) SHA1(82f4586e888f2550c53bfdc93a53791a595e05bd) )
	ROM_LOAD16_WORD_SWAP( "mvc.05a",  0x100000, 0x80000, CRC(2d8c8e86) SHA1(b07d640a734c5d336054ed05195786224c9a6cd4) )
	ROM_LOAD16_WORD_SWAP( "mvc.06a",  0x180000, 0x80000, CRC(8528e1f5) SHA1(cd065c05268ab581b05676da544baf6af642acac) )
	ROM_LOAD16_WORD_SWAP( "mvc.07",   0x200000, 0x80000, CRC(c3baa32b) SHA1(d35589847e0753e869ffcd7c3abed925bfdb0fa2) )
	ROM_LOAD16_WORD_SWAP( "mvc.08",   0x280000, 0x80000, CRC(bc002fcd) SHA1(0b6735a071a9274f7ab25c743271fc30411fe819) )
	ROM_LOAD16_WORD_SWAP( "mvc.09",   0x300000, 0x80000, CRC(c67b26df) SHA1(6e9969246c57269d7ba0992a5cc319c8910bf8a9) )
	ROM_LOAD16_WORD_SWAP( "mvc.10",   0x380000, 0x80000, CRC(0fdd1e26) SHA1(5fa684d823b4f4eec61ed9e9b8938af5272ae1ed) )

	ROM_REGION( 0x100000, "expansion", ROMREGION_ERASEFF ) // CPS-2X 68k expansion chip @0xD30000 (plaintext; host-side build products, blitted at runtime)

	// CPS-2X MSHX CHIP @0x710000: the converted msh characters. A BUILT chip in mvsc's dialect
	// (VS group ids, 16-byte records, mvsc struct offsets), never a mounted msh ROM.
	// ROM_REGION16_BE like the donor chips, so the region's byte order matches the space and
	// the loader blits mshx.bin verbatim in the 68k view -- the A^1 pair swap stays the
	// ':expansion' quirk (see the donor chip note below). ERASEFF: filled at runtime by the
	// loader probe during the build campaign; a release build ROM_LOADs the files here.
	ROM_REGION16_BE( 0xf0000, "mshx", ROMREGION_ERASEFF )
	ROM_REGION16_BE( 0x40000, "mshx2", ROMREGION_ERASEFF ) // CPS-2X mshx growth chip @0x620000
	ROM_REGION16_BE( 0x98000, "convo", ROMREGION_ERASEFF ) // CPS-2X conversation chip @0x668000: the match-start dialog lane, blitted at runtime
	ROM_REGION16_BE( 0x3000, "status", ROMREGION_ERASEFF ) // CPS-2X status chip @0x665000: the boot screen and the changelog pages, blitted at runtime

	// CPS-2X DONOR CHIP @0x930000: mshvsf's WHOLE program image, byte-for-byte, normalised
	// to the 68k's view by the same 16-bit swapping loader ':maincpu' uses. Donor address A
	// is readable at A + 0x930000. Nothing is excised, packed or reordered.
	// ⚠ ROM_REGION16_BE, NOT ROM_REGION(...,0), and that is a measurement not a
	// preference (tests/probe154_byteorder.lua).  An 8-bit LE-declared region
	// mapped into this BE 16-bit space through an explicit `.region()` gets a
	// byte-lane correction, so the 68k reads region[A^1] -- which is exactly why
	// ':expansion' has to be blitted pair-swapped.  ':maincpu' escapes it only
	// because it is mapped by the implicit `.rom()` path.  Declaring the donor
	// chip 16-bit BE makes the region endianness match the space, the correction
	// disappears, and ROM_LOAD16_WORD_SWAP then puts donor address A at 68k
	// address A + 0x930000 with plain big-endian words -- one translation, no
	// byte-order asymmetry, which is the whole point of mounting the image.
	ROM_REGION16_BE( 0x400000, "donor", 0 )
	ROM_LOAD16_WORD_SWAP( "mvse.03f", 0x000000, 0x80000, CRC(b72dc199) SHA1(61bd581ea4b969298f8a39fe03023b5456cac750) )
	ROM_LOAD16_WORD_SWAP( "mvse.04f", 0x080000, 0x80000, CRC(6ef799f9) SHA1(2d45dbf7bc277b84c6bcd9615ab3b80c42af7781) )
	ROM_LOAD16_WORD_SWAP( "mvs.05a",  0x100000, 0x80000, CRC(1a5de0cb) SHA1(738a27f83704c208d36d73bf766d861ef2d51a89) )
	ROM_LOAD16_WORD_SWAP( "mvs.06a",  0x180000, 0x80000, CRC(959f3030) SHA1(fbbaa915324815246738f3426232e623f039ce26) )
	ROM_LOAD16_WORD_SWAP( "mvs.07b",  0x200000, 0x80000, CRC(7f915bdb) SHA1(683da09c5ba55e31b59aa95a8e13c45dc574ab3c) )
	ROM_LOAD16_WORD_SWAP( "mvs.08a",  0x280000, 0x80000, CRC(c2813884) SHA1(49e5d4bc48f90c8146cb6aafb9240aff0119f1a7) )
	ROM_LOAD16_WORD_SWAP( "mvs.09b",  0x300000, 0x80000, CRC(3ba08818) SHA1(9ab132a3cac55fcccebe6c99b6fb0ba1305f8f6e) )
	ROM_LOAD16_WORD_SWAP( "mvs.10b",  0x380000, 0x80000, CRC(cf0dba98) SHA1(f4c1f8a6e7a79ecc6241d5268b3039f8a09ea516) )

	// CPS-2X DONOR #2 CHIP: xmvsf's WHOLE program image (3.5MB, seven files, laid exactly as
	// ROM_START(xmvsf) lays them), same 16-bit BE declaration and swapping loader as ':donor'.
	// Mapped through two windows: region 0x000000-0x1BFFFF at 68k 0xE30000, region
	// 0x1C0000-0x37FFFF at 68k 0x410000 (see cps2x_map).
	ROM_REGION16_BE( 0x380000, "donor2", 0 )
	ROM_LOAD16_WORD_SWAP( "xvse.03f", 0x000000, 0x80000, CRC(db06413f) SHA1(c6d8aa1e43fc541e5b4e938258f27ab9ee30ca33) )
	ROM_LOAD16_WORD_SWAP( "xvse.04f", 0x080000, 0x80000, CRC(ef015aef) SHA1(d3504cb8c38f720b1f4528157266db60c8c6c075) )
	ROM_LOAD16_WORD_SWAP( "xvs.05a",  0x100000, 0x80000, CRC(7db6025d) SHA1(2d74f48f83f45359bfaca28ab686625766af12ee) )
	ROM_LOAD16_WORD_SWAP( "xvs.06a",  0x180000, 0x80000, CRC(e8e2c75c) SHA1(929408cb5d98e95cec75ea58e4701b0cbdbcd016) )
	ROM_LOAD16_WORD_SWAP( "xvs.07",   0x200000, 0x80000, CRC(08f0abed) SHA1(ef16c376232dba63b0b9bc3aa0640f9001ccb68a) )
	ROM_LOAD16_WORD_SWAP( "xvs.08",   0x280000, 0x80000, CRC(81929675) SHA1(19cf7afbc1daaefec40195e40ba74970f3906a1c) )
	ROM_LOAD16_WORD_SWAP( "xvs.09",   0x300000, 0x80000, CRC(9641f36b) SHA1(dcba3482d1ba37ccfb30d402793ee063c6621aed) )

	// ROMREGION_ERASEFF is load-bearing: romload only zero-fills regions of 4MB and under.
	// The fourth 32MB (banks 12-15, tile codes 0xC0000-0xFFFFF) is the mshx tile space: it
	// stays ERASEFF (an all-0xFF tile is every pixel at pen 15 = transparent, so an
	// unfilled code draws nothing) and is blitted at runtime like the HUD plates; a release
	// build ROM_LOADs mshx tile files at 0x6000000. cps2_gfx_decode unshuffles the region in
	// 0x200000 steps whatever its size, and cps2_render_sprites already composes tile-code
	// bits 18 and 19 from y[15] and x[10], so the growth itself is the whole gfx change.
	ROM_REGION( 0x8000000, "gfx", ROMREGION_ERASEFF ) // 128MB: stock 32MB (banks 0-3) + mshvsf's whole 32MB as CPS-2X banks 4-7 + xmvsf's whole 32MB as banks 8-11 + mshx as banks 12-15
	ROM_LOAD64_WORD( "mvc.13m",   0x0000000, 0x400000, CRC(fa5f74bc) SHA1(79a619248938a85ce4f7794a704647b9cf564fbc) )
	ROM_LOAD64_WORD( "mvc.15m",   0x0000002, 0x400000, CRC(71938a8f) SHA1(6982f7203458c1c46a1c1c13c0d0f2a5e109d271) )
	ROM_LOAD64_WORD( "mvc.17m",   0x0000004, 0x400000, CRC(92741d07) SHA1(ddfd70eab7c983ab452194b1860059f8ad694459) )
	ROM_LOAD64_WORD( "mvc.19m",   0x0000006, 0x400000, CRC(bcb72fc6) SHA1(46ab98dcdf6f5d611646a22a7355939ef5b2bbe5) )
	ROM_LOAD64_WORD( "mvc.14m",   0x1000000, 0x400000, CRC(7f1df4e4) SHA1(ede92b31c1fe87f91b4fe74ac211f2fb5f863bc2) )
	ROM_LOAD64_WORD( "mvc.16m",   0x1000002, 0x400000, CRC(90bd3203) SHA1(ed83208c486ea0f407b7e5d16a8cf242a6f73774) )
	ROM_LOAD64_WORD( "mvc.18m",   0x1000004, 0x400000, CRC(67aaf727) SHA1(e0e69104e31d2c41e18c0d24e9ab962406a7ca9a) )
	ROM_LOAD64_WORD( "mvc.20m",   0x1000006, 0x400000, CRC(8b0bade8) SHA1(c5732361bb4bf284c4d12a82ac2c5750b1f9d441) )
	// donor object ROM, whole: mshvsf tile c -> mvscextra tile code 0x40000 + c
	ROM_LOAD64_WORD( "mvs.13m",   0x2000000, 0x400000, CRC(29b05fd9) SHA1(e8fdb1ee5515a560eb4256ae4fd99bb1192e1a87) )
	ROM_LOAD64_WORD( "mvs.15m",   0x2000002, 0x400000, CRC(faddccf1) SHA1(4ed03ea91883a0413325f57edcc1614120b5922c) )
	ROM_LOAD64_WORD( "mvs.17m",   0x2000004, 0x400000, CRC(97aaf4c7) SHA1(6a054921cc14fe080cb3f62c391f8ae3cc7e8ba9) )
	ROM_LOAD64_WORD( "mvs.19m",   0x2000006, 0x400000, CRC(cb70e915) SHA1(da4d2480d348ac6dfd01256a88f4f3db8357ae46) )
	ROM_LOAD64_WORD( "mvs.14m",   0x3000000, 0x400000, CRC(b3b1972d) SHA1(0f2c3fb7de014181ee481ec35d0578b2c116c2dc) )
	ROM_LOAD64_WORD( "mvs.16m",   0x3000002, 0x400000, CRC(08aadb5d) SHA1(3a2c222eca3e7df80ce69951b3db6442312751a4) )
	ROM_LOAD64_WORD( "mvs.18m",   0x3000004, 0x400000, CRC(c1228b35) SHA1(7afdfb552888c79d0fbb30242b3d917b87fad57a) )
	ROM_LOAD64_WORD( "mvs.20m",   0x3000006, 0x400000, CRC(366cc6c2) SHA1(6f2a789087c8e404c5227b927fa8328c03593243) )
	// donor #2 object ROM, whole: xmvsf tile c -> mvscextra tile code 0x80000 + c
	ROM_LOAD64_WORD( "xvs.13m",   0x4000000, 0x400000, CRC(f6684efd) SHA1(c0a2f3a9e82ab8b084a500aec71ac633e947328c) )
	ROM_LOAD64_WORD( "xvs.15m",   0x4000002, 0x400000, CRC(29109221) SHA1(898b8f678fd03c462ce0d8eb7fb3441ef601085b) )
	ROM_LOAD64_WORD( "xvs.17m",   0x4000004, 0x400000, CRC(92db3474) SHA1(7b6f4c8ebfdac167b25f35029068b6253c141fe6) )
	ROM_LOAD64_WORD( "xvs.19m",   0x4000006, 0x400000, CRC(3733473c) SHA1(6579da7145c95b3ad00844a5fc8c2e22c23365e2) )
	ROM_LOAD64_WORD( "xvs.14m",   0x5000000, 0x400000, CRC(bcac2e41) SHA1(838ff24f7e8543a787a55a5d592c9517ce3b8b93) )
	ROM_LOAD64_WORD( "xvs.16m",   0x5000002, 0x400000, CRC(ea04a272) SHA1(cd7c79037b5b4a39bef5156433e984dc4dc2c081) )
	ROM_LOAD64_WORD( "xvs.18m",   0x5000004, 0x400000, CRC(b0def86a) SHA1(da3a6705ea7050fc5c2c10d33400ed67be9f455d) )
	ROM_LOAD64_WORD( "xvs.20m",   0x5000006, 0x400000, CRC(4b40ff9f) SHA1(9a981d442132efff09a27408d74646ba357c7357) )

	// CPS-2X SOUND CHIP: 0x210000 instead of QSOUND_SIZE. The stock ROMs load at their stock
	// offsets and nothing about them moves; what grows is the BANKED half. 128 banks of 0x4000
	// from region 0x10000 give the Z80 flat 0x008000-0x207FFF -- which is exactly the ceiling
	// its own bank arithmetic reaches (`(flat>>14)-2` into an 8-bit accumulator, `or $80` at
	// every D003 write, so banks 0x00-0x7F are the distinguishable set). Stock content ends at
	// flat 0x40000; everything above it is zero and is where the extension's relocated tables
	// and the banked script arena live. See cps2x_qsound_banksw_w and atlas/sound-capacity.md.
	ROM_REGION( 0x210000, "audiocpu", 0 ) // 64k for the audio CPU (+128 banks)
	ROM_LOAD( "mvc.01",   0x00000, 0x08000, CRC(41629e95) SHA1(36925c05b5fdcbe43283a882d021e5360c947061) )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvc.02",   0x28000, 0x20000, CRC(963abf6b) SHA1(6b784870e338701cefabbbe4669984b5c4e8a9a5) )

	ROM_REGION( 0x2000000, "qsound", ROMREGION_ERASEFF ) // 32MB: stock 8MB (banks 0x00-0x7F) + mshvsf's whole 8MB as banks 0x80-0xFF + xmvsf's whole 4MB as banks 0x100-0x13F (page 1)
	ROM_LOAD16_WORD_SWAP( "mvc.11m",   0x000000, 0x400000, CRC(850fe663) SHA1(81e519d05a08855f242ea2e17ee0859b449db895) )
	ROM_LOAD16_WORD_SWAP( "mvc.12m",   0x400000, 0x400000, CRC(7ccb1896) SHA1(74caadf3282fcc6acffb1bbe3734106f81124121) )
	// donor sample ROM, whole: donor bank b -> mvscextra bank b + 0x80
	ROM_LOAD16_WORD_SWAP( "mvs.11m",   0x800000, 0x400000, CRC(86219770) SHA1(4e5b68d382a5aa37f8b0b6434c53a2b95f5f9a4d) )
	ROM_LOAD16_WORD_SWAP( "mvs.12m",   0xc00000, 0x400000, CRC(f2fd7f68) SHA1(28a30d55d3eaf963006c7cbe7c288099cd3ba536) )
	// donor #2 sample ROM, whole: xmvsf bank b -> mvscextra bank 0x100 + b (sample page 1)
	ROM_LOAD16_WORD_SWAP( "xvs.11m",   0x1000000, 0x200000, CRC(9cadcdbc) SHA1(64d3bd53b04daec84c9af4aa3ff010867b3d306d) )
	ROM_LOAD16_WORD_SWAP( "xvs.12m",   0x1200000, 0x200000, CRC(7b11e460) SHA1(a581c84acaaf0ce056841c15a6f36889e88be68d) )

	ROM_REGION( 0x20, "key", 0 )
	ROM_LOAD( "mvsc.key",     0x000000, 0x000014, CRC(7e101e09) SHA1(9d725a7c6bbc20e46f749eaec4bab15b0195077a) )
ROM_END

// mvscextrab: the BAKED mvscextra -- the same CPS-2X board and machine config, but the build lives in the ROM files
// (tools/excise/bake.py writes them; tools/excise/release.py regenerates this block per release, checksums included).
// It boots natively with no Lua loader: the event build.  mvscextra stays the dev set, stock ROMs + the Lua loader.
// Every chip is an `mvx_` file in the set's own folder (rom/releases/mvscextra-vX.Y/mvscextrab/); the chips the dev
// set leaves ERASEFF for the loader (expansion, mshx, mshx2, gfx banks 12-15, the Z80's flat space above the stock
// ROMs, QSound page-1 banks 0x140-0x17F) are real ROM_LOAD lines here.  A clone of mvsc (MAME has no clone of a clone).
// ==== BEGIN GENERATED BY tools/excise/release.py: the baked set (do not edit by hand) ====
// mvscextrab: mvscextra with the build IN THE ROM FILES (tools/excise/bake.py over out_v0.9.4, 2026-10-25).
// Same CPS-2X board as mvscextra; no Lua loader.  Regenerated by tools/excise/release.py per release.
ROM_START( mvscextrab )
	ROM_REGION( CODE_SIZE, "maincpu", 0 )
	ROM_LOAD16_WORD_SWAP( "mvx_mvce.03a", 0x000000, 0x80000, CRC(448c1999) SHA1(dbe04c1e990dc6ec211ff44b1e76594e91141ff9) )  // stock mvce.03a
	ROM_LOAD16_WORD_SWAP( "mvx_mvce.04a", 0x080000, 0x80000, CRC(f63da575) SHA1(49cc66ddf46f6ebcd1ae06e5950f5712dca477e8) )  // stock mvce.04a
	ROM_LOAD16_WORD_SWAP( "mvx_mvc.05a", 0x100000, 0x80000, CRC(2d8c8e86) SHA1(b07d640a734c5d336054ed05195786224c9a6cd4) )  // stock mvc.05a
	ROM_LOAD16_WORD_SWAP( "mvx_mvc.06a", 0x180000, 0x80000, CRC(8528e1f5) SHA1(cd065c05268ab581b05676da544baf6af642acac) )  // stock mvc.06a
	ROM_LOAD16_WORD_SWAP( "mvx_mvc.07", 0x200000, 0x80000, CRC(c3baa32b) SHA1(d35589847e0753e869ffcd7c3abed925bfdb0fa2) )  // stock mvc.07
	ROM_LOAD16_WORD_SWAP( "mvx_mvc.08", 0x280000, 0x80000, CRC(bc002fcd) SHA1(0b6735a071a9274f7ab25c743271fc30411fe819) )  // stock mvc.08
	ROM_LOAD16_WORD_SWAP( "mvx_mvc.09", 0x300000, 0x80000, CRC(c67b26df) SHA1(6e9969246c57269d7ba0992a5cc319c8910bf8a9) )  // stock mvc.09
	ROM_LOAD16_WORD_SWAP( "mvx_mvc.10", 0x380000, 0x80000, CRC(0fdd1e26) SHA1(5fa684d823b4f4eec61ed9e9b8938af5272ae1ed) )  // stock mvc.10

	ROM_REGION( 0x100000, "expansion", ROMREGION_ERASEFF )
	ROM_LOAD( "mvx_exp.bin", 0x000000, 0x100000, CRC(d64eb5b1) SHA1(1fc8a8fed7bed5a2959cbd4e843d849fa66d007b) )  // CPS-2X build chip

	ROM_REGION16_BE( 0xf0000, "mshx", ROMREGION_ERASEFF )
	ROM_LOAD16_WORD_SWAP( "mvx_mshx.bin", 0x000000, 0xf0000, CRC(b150ca52) SHA1(b453ded108946372cfd31e1ad0b314c3bef1b265) )  // CPS-2X build chip

	ROM_REGION16_BE( 0x40000, "mshx2", ROMREGION_ERASEFF )
	ROM_LOAD16_WORD_SWAP( "mvx_mshx2.bin", 0x000000, 0x40000, CRC(5895f1fd) SHA1(ddd74395011a90b68ae982afa6ad4b9f594a711b) )  // CPS-2X build chip

	ROM_REGION16_BE( 0x98000, "convo", ROMREGION_ERASEFF )
	ROM_LOAD16_WORD_SWAP( "mvx_convo.bin", 0x000000, 0x98000, CRC(2237f97b) SHA1(ab2da4ae903fc621d645b38e98f8251b38d8c187) )  // CPS-2X build chip

	ROM_REGION16_BE( 0x3000, "status", ROMREGION_ERASEFF )
	ROM_LOAD16_WORD_SWAP( "mvx_status.bin", 0x000000, 0x3000, CRC(414430c6) SHA1(1083e3ce5e4c2dfaef0084c14b7fa1b08385ef98) )  // CPS-2X build chip

	ROM_REGION16_BE( 0x400000, "donor", 0 )
	ROM_LOAD16_WORD_SWAP( "mvx_mvse.03f", 0x000000, 0x80000, CRC(b72dc199) SHA1(61bd581ea4b969298f8a39fe03023b5456cac750) )  // stock mvse.03f
	ROM_LOAD16_WORD_SWAP( "mvx_mvse.04f", 0x080000, 0x80000, CRC(ebef4e12) SHA1(ddab190c1757b82672a680a48879e78ec29afa60) )  // stock mvse.04f
	ROM_LOAD16_WORD_SWAP( "mvx_mvs.05a", 0x100000, 0x80000, CRC(7fa08e94) SHA1(b163551147081b8f3304e916fe32d3789622fb8a) )  // stock mvs.05a
	ROM_LOAD16_WORD_SWAP( "mvx_mvs.06a", 0x180000, 0x80000, CRC(959f3030) SHA1(fbbaa915324815246738f3426232e623f039ce26) )  // stock mvs.06a
	ROM_LOAD16_WORD_SWAP( "mvx_mvs.07b", 0x200000, 0x80000, CRC(3ead2856) SHA1(8f47f807c966959506ec2a942e8307a5cd2369e7) )  // stock mvs.07b
	ROM_LOAD16_WORD_SWAP( "mvx_mvs.08a", 0x280000, 0x80000, CRC(7b21e138) SHA1(6eaf92dcbd2aa4c7a02679eab43a02a1fcf3af3a) )  // stock mvs.08a
	ROM_LOAD16_WORD_SWAP( "mvx_mvs.09b", 0x300000, 0x80000, CRC(5c537d8a) SHA1(febfe39d635f99710e1e93c30132bfa74d28ab88) )  // stock mvs.09b
	ROM_LOAD16_WORD_SWAP( "mvx_mvs.10b", 0x380000, 0x80000, CRC(40172eba) SHA1(d7eadf03faff055e29d1b50f01dbd31b82b1524f) )  // stock mvs.10b

	ROM_REGION16_BE( 0x380000, "donor2", 0 )
	ROM_LOAD16_WORD_SWAP( "mvx_xvse.03f", 0x000000, 0x80000, CRC(db06413f) SHA1(c6d8aa1e43fc541e5b4e938258f27ab9ee30ca33) )  // stock xvse.03f
	ROM_LOAD16_WORD_SWAP( "mvx_xvse.04f", 0x080000, 0x80000, CRC(f8d6ff21) SHA1(397db0e009606c0a6ec7c3dd12bd6ca9c982557c) )  // stock xvse.04f
	ROM_LOAD16_WORD_SWAP( "mvx_xvs.05a", 0x100000, 0x80000, CRC(7db6025d) SHA1(2d74f48f83f45359bfaca28ab686625766af12ee) )  // stock xvs.05a
	ROM_LOAD16_WORD_SWAP( "mvx_xvs.06a", 0x180000, 0x80000, CRC(9c128a5c) SHA1(a81bdba7677900329487bbd8d579024f8fc6b6e2) )  // stock xvs.06a
	ROM_LOAD16_WORD_SWAP( "mvx_xvs.07", 0x200000, 0x80000, CRC(27b0efda) SHA1(45fada8a2aeb4d9b13cc668ce5eb7c8bde0c319a) )  // stock xvs.07
	ROM_LOAD16_WORD_SWAP( "mvx_xvs.08", 0x280000, 0x80000, CRC(3049022e) SHA1(796b3302de9aa1bd4a7758db4c7b0181d7e9fcc9) )  // stock xvs.08
	ROM_LOAD16_WORD_SWAP( "mvx_xvs.09", 0x300000, 0x80000, CRC(5b9b3cbc) SHA1(0ef592194330ee2b8b5f544425cafe03b5c0cbb5) )  // stock xvs.09

	ROM_REGION( 0x8000000, "gfx", ROMREGION_ERASEFF )
	ROM_LOAD64_WORD( "mvx_mvc.13m", 0x0000000, 0x400000, CRC(7669eff2) SHA1(f27097b33c3fb4fbfabffa938b9e0389ba64feb0) )  // stock mvc.13m
	ROM_LOAD64_WORD( "mvx_mvc.15m", 0x0000002, 0x400000, CRC(06a156fc) SHA1(0374dc5654121be286b3476d1c1d5e5e4fec0e3c) )  // stock mvc.15m
	ROM_LOAD64_WORD( "mvx_mvc.17m", 0x0000004, 0x400000, CRC(272f8719) SHA1(664ba4c6b62840f49407be37639bad84c34f8798) )  // stock mvc.17m
	ROM_LOAD64_WORD( "mvx_mvc.19m", 0x0000006, 0x400000, CRC(8c6d3bb7) SHA1(72a726e385d3eb4fa5cbca7b39984bc71ae693f1) )  // stock mvc.19m
	ROM_LOAD64_WORD( "mvx_mvc.14m", 0x1000000, 0x400000, CRC(7f1df4e4) SHA1(ede92b31c1fe87f91b4fe74ac211f2fb5f863bc2) )  // stock mvc.14m
	ROM_LOAD64_WORD( "mvx_mvc.16m", 0x1000002, 0x400000, CRC(90bd3203) SHA1(ed83208c486ea0f407b7e5d16a8cf242a6f73774) )  // stock mvc.16m
	ROM_LOAD64_WORD( "mvx_mvc.18m", 0x1000004, 0x400000, CRC(67aaf727) SHA1(e0e69104e31d2c41e18c0d24e9ab962406a7ca9a) )  // stock mvc.18m
	ROM_LOAD64_WORD( "mvx_mvc.20m", 0x1000006, 0x400000, CRC(8b0bade8) SHA1(c5732361bb4bf284c4d12a82ac2c5750b1f9d441) )  // stock mvc.20m
	ROM_LOAD64_WORD( "mvx_mvs.13m", 0x2000000, 0x400000, CRC(377a36f5) SHA1(22a2c3ebcb543fe87989431f013335676fbd7d58) )  // stock mvs.13m
	ROM_LOAD64_WORD( "mvx_mvs.15m", 0x2000002, 0x400000, CRC(faddccf1) SHA1(4ed03ea91883a0413325f57edcc1614120b5922c) )  // stock mvs.15m
	ROM_LOAD64_WORD( "mvx_mvs.17m", 0x2000004, 0x400000, CRC(4c70d821) SHA1(c0f46ef60960c5dbe23b804df952e0c2e542f77c) )  // stock mvs.17m
	ROM_LOAD64_WORD( "mvx_mvs.19m", 0x2000006, 0x400000, CRC(cb70e915) SHA1(da4d2480d348ac6dfd01256a88f4f3db8357ae46) )  // stock mvs.19m
	ROM_LOAD64_WORD( "mvx_mvs.14m", 0x3000000, 0x400000, CRC(867d772b) SHA1(625c137b163769647c7ec3122d179eaa8348be1a) )  // stock mvs.14m
	ROM_LOAD64_WORD( "mvx_mvs.16m", 0x3000002, 0x400000, CRC(bccc54c4) SHA1(cfab87a1baf90b55b7f2dc1ae6dddc9301a6b533) )  // stock mvs.16m
	ROM_LOAD64_WORD( "mvx_mvs.18m", 0x3000004, 0x400000, CRC(b5184f74) SHA1(a6e7fe3bd8e007381576482cd6d9b26f9d6c1829) )  // stock mvs.18m
	ROM_LOAD64_WORD( "mvx_mvs.20m", 0x3000006, 0x400000, CRC(4dd37b72) SHA1(bcd187a9404151733153432528356e69c747574f) )  // stock mvs.20m
	ROM_LOAD64_WORD( "mvx_xvs.13m", 0x4000000, 0x400000, CRC(cd05fdab) SHA1(600972af23c88f2f6da861093d59ac392bf83f3c) )  // stock xvs.13m
	ROM_LOAD64_WORD( "mvx_xvs.15m", 0x4000002, 0x400000, CRC(29109221) SHA1(898b8f678fd03c462ce0d8eb7fb3441ef601085b) )  // stock xvs.15m
	ROM_LOAD64_WORD( "mvx_xvs.17m", 0x4000004, 0x400000, CRC(1c248c1c) SHA1(6f92e1735b0b2e1ffb72d0cdb3a4aef678f5bd69) )  // stock xvs.17m
	ROM_LOAD64_WORD( "mvx_xvs.19m", 0x4000006, 0x400000, CRC(3733473c) SHA1(6579da7145c95b3ad00844a5fc8c2e22c23365e2) )  // stock xvs.19m
	ROM_LOAD64_WORD( "mvx_xvs.14m", 0x5000000, 0x400000, CRC(bcac2e41) SHA1(838ff24f7e8543a787a55a5d592c9517ce3b8b93) )  // stock xvs.14m
	ROM_LOAD64_WORD( "mvx_xvs.16m", 0x5000002, 0x400000, CRC(ea04a272) SHA1(cd7c79037b5b4a39bef5156433e984dc4dc2c081) )  // stock xvs.16m
	ROM_LOAD64_WORD( "mvx_xvs.18m", 0x5000004, 0x400000, CRC(b0def86a) SHA1(da3a6705ea7050fc5c2c10d33400ed67be9f455d) )  // stock xvs.18m
	ROM_LOAD64_WORD( "mvx_xvs.20m", 0x5000006, 0x400000, CRC(4b40ff9f) SHA1(9a981d442132efff09a27408d74646ba357c7357) )  // stock xvs.20m
	ROM_LOAD64_WORD( "mvx_mshx.13m", 0x6000000, 0x400000, CRC(0131cb5b) SHA1(5ca3957d704a6e3a7ae5b0facf0faf4cb4ae3a4a) )  // CPS-2X build chip
	ROM_LOAD64_WORD( "mvx_mshx.15m", 0x6000002, 0x400000, CRC(1078284d) SHA1(a44a9015bc8a343ccd440185901230bebde68abe) )  // CPS-2X build chip
	ROM_LOAD64_WORD( "mvx_mshx.17m", 0x6000004, 0x400000, CRC(60baec41) SHA1(02c4bf33d8c2ce12523a0e34db4345e96495b4fd) )  // CPS-2X build chip
	ROM_LOAD64_WORD( "mvx_mshx.19m", 0x6000006, 0x400000, CRC(a1b4244b) SHA1(ae7b00b4ae83e75af7e5ef5f4524454c6cb9e363) )  // CPS-2X build chip
	ROM_LOAD64_WORD( "mvx_mshx.14m", 0x7000000, 0x400000, CRC(de00bd92) SHA1(1e3d2dcc957fe7b5ae0ba05e0f6ced64d34431c0) )  // CPS-2X build chip
	ROM_LOAD64_WORD( "mvx_mshx.16m", 0x7000002, 0x400000, CRC(3e08dcb7) SHA1(fdaa8aa83decded1ac425255d47feab7c3461740) )  // CPS-2X build chip
	ROM_LOAD64_WORD( "mvx_mshx.18m", 0x7000004, 0x400000, CRC(5e366607) SHA1(57afcfc234879a798963a47a4e7f7c5961747379) )  // CPS-2X build chip
	ROM_LOAD64_WORD( "mvx_mshx.20m", 0x7000006, 0x400000, CRC(2b2e701f) SHA1(61d952dd482bc4563d7d01f75bdd05818441be43) )  // CPS-2X build chip

	ROM_REGION( 0x210000, "audiocpu", 0 )
	ROM_LOAD( "mvx_mvc.01", 0x000000, 0x8000, CRC(259a4ae6) SHA1(564e2367398b8fbdf38727783c90a666d99d3b9d) )  // stock mvc.01
	ROM_CONTINUE( 0x10000, 0x18000 )
	ROM_LOAD( "mvx_mvc.02", 0x028000, 0x20000, CRC(423b9157) SHA1(85627b6fa8e73b2a9d1e2ab2aff37409c1f03b83) )  // stock mvc.02
	ROM_LOAD( "mvx_snd.ext", 0x048000, 0x1c8000, CRC(f55f8395) SHA1(1344b68523957cfca752c3dd5ef3ad9f3ba483a0) )  // CPS-2X build chip

	ROM_REGION( 0x2000000, "qsound", ROMREGION_ERASEFF )
	ROM_LOAD16_WORD_SWAP( "mvx_mvc.11m", 0x0000000, 0x400000, CRC(850fe663) SHA1(81e519d05a08855f242ea2e17ee0859b449db895) )  // stock mvc.11m
	ROM_LOAD16_WORD_SWAP( "mvx_mvc.12m", 0x0400000, 0x400000, CRC(7ccb1896) SHA1(74caadf3282fcc6acffb1bbe3734106f81124121) )  // stock mvc.12m
	ROM_LOAD16_WORD_SWAP( "mvx_mvs.11m", 0x0800000, 0x400000, CRC(86219770) SHA1(4e5b68d382a5aa37f8b0b6434c53a2b95f5f9a4d) )  // stock mvs.11m
	ROM_LOAD16_WORD_SWAP( "mvx_mvs.12m", 0x0c00000, 0x400000, CRC(f2fd7f68) SHA1(28a30d55d3eaf963006c7cbe7c288099cd3ba536) )  // stock mvs.12m
	ROM_LOAD16_WORD_SWAP( "mvx_xvs.11m", 0x1000000, 0x200000, CRC(9cadcdbc) SHA1(64d3bd53b04daec84c9af4aa3ff010867b3d306d) )  // stock xvs.11m
	ROM_LOAD16_WORD_SWAP( "mvx_xvs.12m", 0x1200000, 0x200000, CRC(7b11e460) SHA1(a581c84acaaf0ce056841c15a6f36889e88be68d) )  // stock xvs.12m
	ROM_LOAD16_WORD_SWAP( "mvx_mshx.11m", 0x1400000, 0x200000, CRC(37ac6d30) SHA1(ec67421fbf4a08a686e76792cb35e9cbf04d022d) )  // CPS-2X build chip
	ROM_LOAD16_WORD_SWAP( "mvx_mshx.12m", 0x1600000, 0x200000, CRC(de092570) SHA1(a03d0df901f6ea79685eaed67db65bee14ec29c6) )  // CPS-2X build chip

	ROM_REGION( 0x20, "key", 0 )
	ROM_LOAD( "mvx_mvsc.key", 0x000000, 0x14, CRC(7e101e09) SHA1(9d725a7c6bbc20e46f749eaec4bab15b0195077a) )  // stock mvsc.key
ROM_END
// ==== END GENERATED BY tools/excise/release.py ====

/*************************************
 *
 *  Games initialization
 *
 *************************************/

void cps2x_state::init_digital_volume()
{
	m_cps2digitalvolumelevel = 39; // maximum
	m_cps2disabledigitalvolume = 0;

	// create a timer to update our volume state from the fake switches - read it every 6 frames or so to enable some granularity
	m_digital_volume_timer = timer_alloc(FUNC(cps2x_state::cps2_update_digital_volume), this);
	m_digital_volume_timer->adjust(attotime::from_msec(100), 0, attotime::from_msec(100));

	save_item(NAME(m_cps2digitalvolumelevel));
}


void cps2x_state::init_cps2_video()
{
	cps2_gfx_decode();
	init_rasters(); // capcom/cps1.cpp
}


void cps2x_state::init_cps2crypt()
{
	if (m_region_key)
	{
		uint16_t decoded[10] = { 0 };
		for (int b = 0; b < 10 * 16; b++)
		{
			int bit = (317 - b) % 160;
			if ((m_region_key->base()[bit / 8] >> ((bit ^ 7) % 8)) & 1)
			{
				decoded[b / 16] |= (0x8000 >> (b % 16));
			}
		}

		const uint32_t key[2] = { ((uint32_t)decoded[0] << 16) | decoded[1], ((uint32_t)decoded[2] << 16) | decoded[3] };
		// decoded[4] == watchdog instruction third word
		// decoded[5] == watchdog instruction second word
		// decoded[6] == watchdog instruction first word
		// decoded[7] == 0x4000 (bits 8 to 23 of CPS2 object output address)
		// decoded[8] == 0x0900

		uint32_t lower, upper;
		if (decoded[9] == 0xffff)
		{
			// On a dead board, the only encrypted range is actually FF0000-FFFFFF.
			// It doesn't start from 0, and it's the upper half of a 128kB bank.
			upper = 0xffffff;
			lower = 0xff0000;
		}
		else
		{
			upper = (((~decoded[9] & 0x3ff) << 14) | 0x3fff) + 1;
			lower = 0;
		}

		logerror("cps2 decrypt 0x%08x,0x%08x,0x%08x,0x%08x\n", key[0], key[1], lower, upper);

		// we have a proper key so use it to decrypt
		cps2_decrypt(machine(), (uint16_t *)memregion("maincpu")->base(), m_decrypted_opcodes, memregion("maincpu")->bytes(), key, lower / 2, upper / 2);
	}
}


void cps2x_state::init_cps2()
{
	// Decrypt the game - see capcom/cps2crypt.cpp
	init_cps2crypt();
	init_cps2nc();
}

// 4-player 2v2 tag mod. Each set supplies the work-RAM address of its two teams' on-point index
// (field +0x220 of each team's ACTIVE fighter struct -- see the cps2_4p_in0_r block comment).
// The addresses are the only per-game part of the mod; everything else is shared. To map a new
// set, disassemble its tag routine to find the active-slot bases, then add an init here.
void cps2x_state::init_vs4p(uint32_t gate1, uint32_t gate2)
{
	m_vs4p_gate[0] = gate1;
	m_vs4p_gate[1] = gate2;
	init_cps2();
}

// Verified: tag routine @0x010250 exchanges FF4000<->FF4800 (team 1) and FF4400<->FF4C00 (team 2).
void cps2x_state::init_xmvsf_4p()  { init_vs4p(0xff4220, 0xff4620); }

// mshvsf uses PERMANENT per-character slots (array at FF3800 stride 0x400; char-id at slot+0x53),
// NOT xmvsf's data-swap. Team1 = starter FF3800 (P1) + partner FF4000 (P3); team2 = FF3C00 (P2) +
// FF4400 (P4). GATE = partner slot +0x00, the fighter's ON-FIELD flag: 0 while benched/off-field,
// 1 while on point. Movement-proven stable (probe100: 100% both states through neutral AND motion).
// NOTE the earlier +0x79 gate (FF4079/FF4479) was WRONG: it is a pose-coupled state byte that
// toggles 0<->nonzero EVERY frame when a pad drives an animation (e.g. holding down), so the mux
// flipped P1<->P3 per frame = "turbo"/stutter input. The old probe22 RAM-diff only sampled two
// FROZEN configs and never caught the per-frame oscillation. +0x00 is decoupled from animation.
void cps2x_state::init_mshvsf_4p() { init_vs4p(0xff4000, 0xff4400); }

// mvsc structure (verified live, probe22): mvsc indexes its fighter structs exactly like mshvsf --
// both read a slot index from +0x94 and compute base + index*0x400 (mvsc base FF3000, mshvsf
// FF3800). Confirming that shared code shape in mvsc's OWN ROM first is what made the carry-over
// valid; porting xmvsf's layout to mshvsf on the same hunch failed completely.
//
// GATE = partner slot +0x00, the ON-FIELD flag. mvsc partner slots are FF3800/FF3C00.
// An earlier init_mvsc_4p used slot +0x79 (FF3879/FF3C79) and has been REMOVED: that byte is
// pose-coupled and toggles 0<->nonzero every frame while a pad drives an animation, so the mux
// flipped P1<->P3 per frame = "turbo"/stutter input. Identical failure to mshvsf's -- see the
// init_mshvsf_4p note. The old probe22 RAM-diff only sampled two FROZEN configs and never caught
// the per-frame oscillation.
void cps2x_state::init_mvsc2v2_4p() { init_vs4p(0xff3800, 0xff3c00); }

// mvscduo (widescreen 4-LIVE Duo variant): all P3/P4 routing is baked into the program ROM by the
// STUB v3 hijack at 0x13698 (routes fighter slot FF3800->port 804050, FF3C00->804052), so the
// driver input mux must stay OUT of the path. Leave m_vs4p_gate at {0,0} (do NOT call init_vs4p):
// with the gate unset, cps2_4p_in0_r/in1_r/in2_r pass P1/P2 through unchanged. Just do the crypto.
void cps2x_state::init_mvscduo() { init_cps2(); }

// CPS-2X (mvscextra): mvsc's proven 2v2 gates (partner-slot on-field flags, same RAM layout as
// stock mvsc) plus the extended-object flag. The flag is what lets the renderer read tile-code
// bit 18 from object-record y[15]; the GAME-side bank table (0x24C7E family) and per-character
// bank bytes (+0x47) decide who actually emits it. With "Play Mode" at 1v1 the 4P mux is inert
// and the set behaves bit-stock.
void cps2x_state::init_mvscextra()
{
	init_vs4p(0xff3800, 0xff3c00);
	save_item(NAME(m_vs4p_playmode_game));
	m_cps2x_ext_obj = true;
	// Extended sprite palette pages 4-5 (object x[11:12], see cps2_render_sprites): the 16x16
	// element is declared with 0x80 colour codes and the renderer takes the code modulo that
	// count, so widen it to the 0xC0 codes the 0xC00-pen palette holds. Codes below 0x80 (sprites
	// 0x00-0x1F, scroll2 0x40-0x5F) resolve to the same pens under either modulus.
	m_gfxdecode->gfx(2)->set_colors(0xc0);
}

void cps2x_state::init_cps2nc()
{
	// Initialize some video elements
	init_cps2_video();

	init_digital_volume();

	// HACK: RAM access waitstates etc. aren't emulated - slow the CPU to compensate
	m_maincpu->set_clock_scale(0.7375);
}

// rearrange the graphics data into the normal order


} // anonymous namespace


/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1996, xmvsf2v2,   xmvsf,  cps2_4p_43,  cps2_4p6b_mode2v2, cps2x_state, init_xmvsf_4p,   ROT0, "TORNOTLUKIN", "X-Men Vs. Street Fighter 2v2 (4 players)", MACHINE_SUPPORTS_SAVE )
GAME( 1997, mshvsf2v2,  mshvsf, cps2_4p_43,  cps2_4p6b_mode2v2, cps2x_state, init_mshvsf_4p,  ROT0, "TORNOTLUKIN", "Marvel Super Heroes Vs. Street Fighter 2v2 (secret characters)", MACHINE_SUPPORTS_SAVE )
GAME( 1998, mvsc2v2,    mvsc,   cps2_4p_43,  cps2_4p6b_mode2v2, cps2x_state, init_mvsc2v2_4p, ROT0, "TORNOTLUKIN", "Marvel Vs. Capcom: Clash of Super Heroes 2v2 (all characters)", MACHINE_SUPPORTS_SAVE )
GAME( 1998, mvscduo,    mvsc,   cps2_4p_duo, cps2_4p6b_duo,     cps2x_state, init_mvscduo,    ROT0, "TORNOTLUKIN", "Marvel Vs. Capcom: Clash of Super Heroes Duo (widescreen)", MACHINE_SUPPORTS_SAVE )
GAME( 1998, mvscextra,  mvsc,   cps2x,       cps2x_mode,        cps2x_state, init_mvscextra,  ROT0, "TORNOTLUKIN", "Marvel Vs. Capcom: Clash of Super Heroes Extra (CPS-2X expanded roster)", MACHINE_SUPPORTS_SAVE )
GAME( 1998, mvscextrab, 0,      cps2x,       cps2x_mode,        cps2x_state, init_mvscextra,  ROT0, "TORNOTLUKIN", "Marvel Vs. Capcom: Clash of Super Heroes Extra (CPS-2X, baked release ROMs)", MACHINE_SUPPORTS_SAVE )
