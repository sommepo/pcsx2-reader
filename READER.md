# PCSX2 Reader: the fork's guide

PCSX2 Reader is a fork of the PCSX2 PlayStation 2 emulator's desktop app (for Windows) that is
to show a game's text as real text: click a word in the game's own picture to look it up, make
Anki cards, keep a backlog and reading statistics. It is the PS2 sibling of Eden Reader
(Switch), Azahar Reader (3DS) and PPSSPP Reader (PSP) and shares their reading layer; its
Android sibling is ARMSX2 Reader (`../armsx2-reader-fork`), which has the same engine and the
same look.

`AGENTS.md` is upstream PCSX2's own guide for agents and stands as it is; this file is the
fork's. If a file `MAINTAINER.md` exists (the maintainer's own checkout), read it after this
one.

## State (2026-10-15): the interface, seventeen games, a Windows build nobody has run

**What is there** is the app's face and the reader's windows: the PS2-style menu in place of
the game list, the console's colours in every window, the reader's menu and its windows
(statistics, supported games, dictionaries, Anki setup, backlog, settings), the popup and the
layer over the game's picture, and under them a text engine that loads a game's profile and
runs it once a frame.

**Seventeen games can be read**: ひぐらしのなく頃に祭 カケラ遊び (SLPM-66913), Remember11
(SLPM-65550), Ever17 Premium Edition (SLPM-65421), Never7 (SLPS-25256), ペルソナ4
(SLPM-66978), and since 2026-10-14 ペルソナ3 (SLPM-66445), ドラゴンクエストV (SLPM-65555),
サイレントヒル2 (SLPM-65051), 真・女神転生III マニアクス (SLPM-65462), DIGITAL DEVIL
SAGA (SLPM-65597), アマガミ (SLPS-25918), EVE burst error PLUS (SLPM-65320), うたわれるもの
散りゆく者への子守唄 (SLPS-25679) and シャドウハーツ (SLPS-25041), and since 2026-10-15 METAL GEAR
SOLID 2 SUBSTANCE (SLPM-67002), METAL GEAR SOLID 3 SNAKE EATER (SLPM-65790) and CLANNAD
(SLPM-66302), whose routes finished elsewhere can be marked ("Routes finished elsewhere"). The two
ペルソナ, 真・女神転生III, DIGITAL DEVIL SAGA and the two METAL GEAR SOLIDs each need a
character table that is not in Git (see "Games"). See "Games" below for what of each is read
and what is not.

**Looked at and not readable** (2026-10-14, again 2026-10-15): ファイナルファンタジーXII (SLPM-66320,
program 280AD120). It starts and plays here. What is known (2026-10-15, from the prologue's
tutorial and Basch's lines): hiragana are one byte, the code point less 0x3003 (て is 0x63),
katakana one byte from 0x91 (ァ) on, a line break is 02, commands begin with 0F, and kanji
and some signs are two bytes with a first byte from 0x10 to 0x1F (、 is 10C3, 。 10C4), in an
order of the game's own that looks like the order kanji first appear in the script (確認 is
1221 1222, 帝国兵 10E9 10E3 10EA). So a profile needs a table of those codes. Not found: the
menus' font is in the GS's memory (a 640-wide 4-bit sheet, cells 20 by 23, kanji in tiers
sorted by reading), but no table from codes to its cells was found; the message font is
drawn into a cache in the GS's memory as two-bit layers, glyph by glyph, and where it comes
from was not found in the EE's or the IOP's memory in a plain one-, two- or four-bit form
(probably compressed). The text renderer's code was not found. The harness can now dump the
GS's memory (`vram PATH`) and the IOP's (`iop PATH`) for whoever goes on. METAL GEAR SOLID 3,
said here on 2026-10-14 to be unreadable, was a misreading: its kana are in Unicode's order
two bytes apart, which the search did not try (see "Games").
ウォレスとグルミット 野菜畑で大ピンチ！ (SLPM-66327, program 3C38D7E1) keeps its text in UTF-16,
but here its films draw black and silent and after the opening one the game goes back to its
title (also with the Skip MPEG game fix), so no text of the game proper was reached; set
aside at the maintainer's word. バイオハザード4 (SLPM-66213, program 60FA8C69) starts and its
memory card notice comes up; paused at the maintainer's word before its text was looked for.

**A Windows build** is made on Linux (`tools/reader/build-windows.sh`,
`package-windows.sh`). It was never started: see "Windows".

**What is not there yet**:

- **Hooks in a game's code** are not carried out by the emulator's recompilers
  (`Ps2Host::InvalidateCode` in `reader/core/emulator.cpp` is where that begins). A profile
  reads memory once a frame (`reader.on_frame`), which is what the siblings prefer anyway.
- **A card's audio** is fed (the mixer's output goes to the engine's audio history, and was
  seen to hold the game's sound), but no card was made with it: whether the piece cut for a
  line is that line's voice is not known.
- **The controller**: the reader can press the console's buttons and rest its sticks
  (`Reader::Emu::PressButton`, `RestSticks`; the harness and touch play use the first), but the cursor
  steered with the sticks and the button that pauses the statistics were not tried, and the
  input filter does not take a pad's input away from the game.
- **The Browser's "Options"** (△; the Menu key or a right click on a game) is shown and
  opens nothing yet.
- **Save sync** (2026-10-10, `reader/save_sync`, the Eden Reader fork's engine and service):
  while a game runs, the first memory card slot holds **a card of that game's own**, an
  ordinary card file `reader-saves/<serial>/card.ps2` in the memory cards folder, whatever
  card the settings name (a new one is unformatted, as every new card of the emulator's:
  the game formats it when it first saves);
  the newest save is fetched before the emulator opens the card and sent after it writes
  one. *Reader, Save sync...* has the service's address and code and settles two devices'
  different saves. A game is named to the service by its serial's four letters and five
  digits (`SLPM-65550` is `534C504D00065550`). With save sync on, the reading statistics go
  to the service too when a game stops. Saves on the settings' own card are not carried
  over: a game starts with an empty card of its own. Tests: `tools/reader/test-savesync.sh`
  (31, against services it starts itself on loopback ports). Seen on Linux without a
  display (2026-10-11): Higurashi formatted its new card and made its save data on it; with
  a service on a loopback port, that card went up from one test install and was fetched by
  another, which had none, before the emulator opened it (the same file, byte for byte, and
  the emulator's log calls it formatted). A first build gave each game a folder card, which
  the game could not format: hence the card file. `READER_NO_OWN_CARD=1` in the environment
  leaves the cards as the settings have them. **Not seen**: two different saves settled in the window, anything on Windows.
- **Where the game's picture is** in the window is worked out from the aspect ratio alone
  (`PictureRect` in `reader/qt/host.cpp`); the emulator's zoom, offsets and integer scaling
  are not followed, and only a game shown in the main window is covered.

## Ground rules

- **Nothing goes to upstream PCSX2 from here.** `AGENTS.md` (upstream's) says so of agents in
  so many words: never an issue, never a pull request. The remote `upstream` cannot be pushed
  to. Do not present this app as PCSX2.
- **Commits say `(AI-assisted)`**, as upstream's guide asks of every message.
- **No game data, no BIOS, in Git, ever.** Discs, the console's BIOS, anything extracted or
  derived from them, savestates, screenshots: under `private/`, which Git ignores. A bundled
  profile holds addresses and decoding logic only. Tests use made-up text. The one exception,
  by the maintainer's word: the README's pictures and animations in `docs/reader/media`.
- **A table made from a game is game data too** (a font's character table, as the two
  ペルソナ, 真・女神転生III and DIGITAL DEVIL SAGA need): it is `private/<game>/<SERIAL>.charset`, the profile asks for it with
  `reader.data("charset")`, and `tools/reader/private-data.sh`, which the build scripts call,
  puts it beside the profile for a build of your own (Git ignores it there). The profile says
  clearly when it is missing. A table can also be put beside a profile of one's own in the
  app's data folder (`reader/profiles/<SERIAL>.charset`).
- **Upstream files are touched as little as possible.** The reader lives in `reader/`. Where
  an upstream file must change, the change is small and carries a comment that begins
  `Reader fork`.
- **Bound what you run.** `tools/reader/limit.sh` runs a command under a memory limit and
  every script in `tools/reader/` uses it; compile jobs are `READER_JOBS` (8). Never run a
  Windows build under Wine.
- **Say what was checked and how**: a test with made-up text, the app on Linux without a
  display, a real Windows computer are three levels, and a note says which.

## Layout

| Path | |
| --- | --- |
| `reader/core/` | The text engine (profiles in Lua, text events, the audio history, the input filter). Knows nothing of the emulator; `emulator.cpp` is the one file that ties it to PCSX2 and `emulator.h` all that upstream files call. |
| `reader/lua/` | Lua 5.4, built as C++. |
| `reader/desktop/` | The reading logic, free of the emulator: dictionaries and lookup, cards and AnkiConnect, statistics, the backlog database. Tests in `tests/`. |
| `reader/qt/` | The reader's windows; `host.cpp` ties them to the emulator's Qt frontend and `install.h` is all that frontend sees. `ps2_home.*` is the PS2-style menu (`Ps2Home`) and the look (`Ps2Look`). |
| `reader/profiles/` | The game scripts (profiles), one `<SERIAL>.lua` per game (`SLPM-66913.lua`): built into the app unless `READER_BUNDLE_PROFILES` is off, and published as the list the apps install from ("Game scripts"). |
| `reader/desktop/script_index.*` | The published list read, a script checked and installed or removed (`qt/reader_games.cpp` is its window). |
| `tools/reader/` | Build, test and run scripts. `env.sh` has every path. |
| `private/` | Ignored by Git. |

Upstream files touched: `CMakeLists.txt` (the reader's folder), `pcsx2/VMManager.cpp` (which
program runs, each frame, shutdown, a loaded savestate), `pcsx2/Pcsx2Config.cpp` (the app's
own data folder, `PCSX2Reader`), `pcsx2-qt/MainWindow.cpp` (the reader installed, the games
for the menu), `pcsx2-qt/Themes.cpp` (the colours after the theme),
`pcsx2-qt/DisplayWidget.cpp` (the reader gets the pointer first), `pcsx2/SPU2/spu2.cpp`
(what the game plays, for the audio history), `pcsx2/SIO/Pad/Pad.cpp` (a press of the
player's, which stops a run to the next choice), and for the Windows build made on Linux
`cmake/SearchForStuff.cmake` (a folder's name in its real letter case) and
`pcsx2/CMakeLists.txt` (Qt's deployment tool only where it exists).

Branch `reader`, based on upstream tag `v2.8.2` (a shallow clone).

## The PS2's look

On by default, as in the siblings. `reader/qt/ps2_home.cpp`:

- **The main menu**: "Browser" and "System Configuration" beside seven lights turning slowly
  in the dark, the chosen entry lit from within; in the bottom right corner what the buttons
  do, with the console's four marks in their colours.
- **Browser**: the games as pictures afloat in a grey-blue haze (a plain case with the
  game's name where there is no cover), the chosen one larger and outlined in light, its
  name and serial in the top right corner, a boxed book on a game whose text can be read.
  Typing narrows the games to those whose name has what was typed.
- **System Configuration**: a list of names with their values on the right: Reading
  Statistics (what was read today), Game Scripts, Dictionaries, Anki, Reader Settings,
  Emulator Settings, Controllers, Game Folders, Game List (which switches this menu off).
- Arrow keys, Enter, Esc or Backspace; the mouse (a click chooses and enters, the wheel
  moves, the right button goes back). **No controller**: the emulator reads its pads only
  while a game runs.
- **Every window has the menu's colours**, and the reader's windows are laid out as pages of
  System Configuration: the page's name in light blue, the page, and "✕ Enter ○ Back" along
  the bottom. *Reader → PS2 system menu as the game list* and *PS2 colours in every window*
  switch the two off and on.

- **The dictionary popup and the pointer over a game are the menu's too** (2026-10-10):
  the popup is black-indigo glass with a thin light-blue edge, each headword lit from
  within, the rule under an entry fading to the right; the pointer is one of the menu's
  lights, a white core in a blue halo, for the mouse and for the controller's cursor alike.
  The plus and the book keep their shapes. The word's mark and the session line are as they
  were. Drawn in `reader/qt/reader_overlay.cpp` ("The console's look").

The menu lies over the emulator's game list and shows whenever the list would; the list is
what it was underneath.

## Build and run (Linux)

Upstream builds with Clang only and needs Qt 6.10. Everything is in one folder outside the
repository (`~/.local/share/ps2-reader`), without root:

1. An environment of conda-forge packages at `~/.local/share/ps2-reader/host`: `clangxx=20
   clang=20 lld=20 llvm-tools=20 compiler-rt=20 gxx_linux-64=14 sysroot_linux-64=2.28 cmake
   ninja ccache pkg-config make git qt6-main=6.10.* sdl3 libpng libjpeg-turbo zlib zstd lz4-c
   libwebp freetype harfbuzz ffmpeg=7.1.* libcurl libpcap fontconfig` and the X11, Wayland,
   D-Bus, udev, ALSA, PulseAudio, OpenGL and Vulkan header packages (`micromamba create -p …
   -c conda-forge …`). FFmpeg 8 does not do: the emulator's video capture is written for 7.
2. `tools/reader/deps-desktop.sh`: libbacktrace, KDDockWidgets, PlutoVG, PlutoSVG, RapidYAML
   and the emulator's patched shaderc, at the versions and checksums upstream's own script
   for Linux names, built into the same folder.

```sh
tools/reader/build-desktop.sh         # build-desktop/bin/pcsx2-qt
tools/reader/test-desktop.sh          # tests of the reading logic (118)
tools/reader/desktop.sh &             # run it without a display (Qt's "offscreen")
python3 tools/reader/dc.py home "home-key Down" "home-key Return" "home-grab private/desktop/shots/a.png"
tools/reader/desktop-stop.sh
```

`dc.py` talks to the running app through a socket that exists only when `desktop.sh` or
`screen.sh` started it (the commands are in `reader/qt/reader_control.cpp`): `home` says what
the menu shows, `home-key` and `home-click` drive it, `home-demo N` fills the Browser with
made-up games, `home-grab` writes a picture of it; `action reader_settings`, `dialog-grab`,
`dialog-close` do the same for the reader's windows (`dialog-button TEXT` presses a button,
`dialog-select TEXT` makes a list's row current). `READER_DESKTOP_HOME` gives a run a home of
its own (and `desktop-stop.sh` stops only the run of that home), `READER_DESKTOP_BUILD` a
build folder of its own: two agents can each build and run without stepping on the other.
`click`, `move`, `press` and `release` go to the game's display surface (a window embedded in
the main window's container), where a real pointer's events arrive; with
`READER_OVERLAY_UNMANAGED=1` the popup and the audio question are kept from the window manager
as the layer is, so under `screen.sh` they stand where the reader puts them. `grab` composes
the game's frame (before presentation: no CRT picture in it), the layer and the popup.

### A game, with a picture

`desktop.sh` has no screen, and the emulator draws a game only to one. `screen.sh` runs the
app on a Weston compositor that draws to nothing, the game drawn by Mesa's software Vulkan
driver (both are the sibling forks' and are named in the script). It is slow but it is the
game. The BIOS goes in `private/desktop/config/PCSX2Reader/bios` and is named in the
settings file beside it; the games folder is `private/desktop/games`.

```sh
tools/reader/screen.sh -- "$PWD/private/desktop/games/game.iso" &
python3 tools/reader/dc.py "pad circle 100" "grab $PWD/private/desktop/shots/a.png" text
python3 tools/reader/dc.py "dump $PWD/private/game/ram.bin"   # the console's main memory
python3 tools/reader/dc.py "profile $PWD/private/game/SERIAL.lua"   # try a profile being written
python3 tools/reader/dc.py audio                             # the audio history: frames kept, how loud
tools/reader/desktop-stop.sh
```

`pad NAME [ms]` presses a button of the first controller (`up right down left triangle
circle cross square select start` and the shoulder buttons); `lua CODE` runs a piece of Lua
in the engine. Paths are absolute: the app's working folder is `private/desktop`.
`state save N` / `state load N` write and load save state slot N (1 to 10) of the running
game, `speed turbo` / `speed normal` take the speed limit off and put it back, `pause on` /
`pause off` hold the game and let it go (the keyboard's hotkeys do not reach the emulator
through this channel); `vram PATH` writes the GS's memory read as one 4-bit texture 1024 wide
(a byte a pixel: each 128 by 128 page is right whatever the texture's real width), `iop PATH`
the IOP's 2 MB.

### On a bare X11 window manager (Batocera)

Three things were needed on a PC running Batocera 42 (X11, Openbox, no desktop), seen there
on 2026-10-10 with Ever17 started from EmulationStation:

- **A build for any processor**: `build-desktop.sh` builds for the machine it runs on
  (`-march=native`, upstream's default) and that program stops with "illegal instruction"
  elsewhere. Configure a second tree with `-DDISABLE_ADVANCE_SIMD=ON` (upstream's own
  portable setting; `build-portable/` was made so by hand).
- **A compositing manager** (`xcompmgr` beside the app): the layer over the game is a
  see-through window, which is a black rectangle over the picture without one.
- **`READER_OVERLAY_UNMANAGED=1`** in the environment: Openbox frames the layer and the
  frame takes every click on the game, so no word could be clicked; with the variable the
  layer is made without the window manager.

With the three, a press of a pointer's button on a word opened its entry. The game has to be
shown in the main window, not full screen (the layer covers only that, as said above).

## The controller's back buttons

`reader/qt/reader_pad` (the same file in every reader fork) reads the buttons no game uses, beside
the emulator: on Linux any controller the system offers that has the "extra" buttons
(`BTN_TRIGGER_HAPPY1` to `5`: what the Steam Controller bridge on a Batocera box makes of the
four grips and the "..." button); on Windows the Steam Controller's (2026) own reports, read
from the device beside whoever else reads it, nothing written. Out of the box: grip 1
**pauses the game** with its picture up and every word still to be looked up, and lets it go
on (Ctrl+G; nothing is said on screen); grip 2 pauses the statistics (Ctrl+T); grip 3 makes a
card of the sentence; grip 4 opens the backlog; "..." the reader's menu. *Reader settings,
Back buttons* has a row for each. The development channel's `back-button N` presses one.

Checked (2026-10-10): on a PC running Batocera, a stand-in controller that sends the bridge's
own reports pressed grips 1 and 2 twice each in a running game: the frame count stood still
and went on again, the statistics' mark came and went. **Not checked**: a real controller in
a hand; grips 3 to 5 there; anything of the Windows part, which has never run.


## Going on to the next choice

For someone who has read part of a visual novel elsewhere (another copy of the game, another
language's release) and wants to give this one the same choices without reading it all
again. *Reader, Go on to the next choice* (Ctrl+N; also a choice for the back buttons) runs
the game as fast as the emulator goes and presses ○ every eighth frame until the profile
reports a choice (an event of the channel `choice`), then lets the game run at its speed
again. The text on the way is not read: the engine keeps it from the reader (no statistics,
no backlog, nothing to point at) and sends only the message that stands under the choice
when it is reached. It stops as well when nothing at all was reported for 15 seconds of the
game's time (a film, the credits, a menu: ○ does there what ○ does, and in a save menu it
saves), at a press of any of the game's buttons on any controller, and at Ctrl+N again. A
message on the game's picture says why it stopped and how many texts were passed.

`reader/core/next_choice.*` decides, frame by frame; `emulator.cpp` does it (the speed, the
button, the player's presses, which `pcsx2/SIO/Pad/Pad.cpp` passes on) and
`Engine::SetQuiet` keeps the text back. It works for any game whose profile reports choices
on the `choice` channel, but **it was only tried with Never7**: a choice the profile does not
report is run through with ○, which in a game where ○ picks the answer under the cursor
picks that one. In Never7 ○ alone picks nothing (the cursor has to be moved first), and the
game itself quick-saves at every choice (its own option, on from the start), which undoes a
wrong answer. The development channel's `next-choice` starts and stops it, `next-choice ?`
says whether it is going.

Checked (2026-10-14, Never7 on Linux, drawn in software): from a new game through 18
choices one after another, a choice of two, three and four answers, the quiz on the tennis
court that asks again with the wrong answers gone; each run 3 to 47 seconds here (up to 286
messages passed), stopping with the choice up and none picked and the message under it
reported (0 to 33 characters counted for the run). A press of □ during a run stopped it.
Started in the game's save menu by mistake, it saved into the first slot until the time
without text was up. **Not tried**: the other games, Windows, the back buttons for it.

## Routes finished elsewhere (2026-10-15)

For someone who has read part of a visual novel in another copy of it (the maintainer: CLANNAD
on Steam) and wants this copy to know: a game's script may offer the game's own progress to be
set by hand, and *Reader, Routes and progress…* (enabled while such a game runs; the
development channel's `progress` lists, `progress ID on|off` marks) shows each thing with a box.
A box changed is written into the game's memory at once by the script; the game keeps it as it
keeps its own progress, and the window's note says how (CLANNAD: its system data, saved from
its menu or after an ending). The profile API (`reader.progress{title, note, items = {{id,
label, group}...}, done = f(id), mark = f(id, on)}`; `mem.w8`, `w16`, `w32`) is version 2 of
the scripts' API (`-- reader: api 2`): an older app shows such a script as needing a newer app.
**A script writes the game's memory only from its `mark`**: anywhere else `mem.w16` is an
error, and the window is the only caller (the game is paused while it is open). Main memory
only, above the kernel's first megabyte. `Engine::Progress`, `Engine::MarkProgress`,
`LuaApi::CollectProgress`; tests with made-up memory in `reader/desktop/tests/progress_test.cpp`.

**A game's own card is now made formatted** (`FormatCard` in `reader/core/emulator.cpp`): the
emulator's new cards are blank, which most games format when they first save, but CLANNAD only
says the card is not formatted and saves nothing. The superblock, the indirect FAT list, the
FAT and an empty root folder are written as the console writes them (the superblock page and
its ECC came out byte for byte those of a card the console had formatted). A card made before
stays as it is.

## Playing by touch (2026-10-16)

For a Windows tablet without a controller (and the same in ARMSX2 Reader on a phone): *Reader
settings, Play by touch* (`reader_touch_play`; **off at first on a computer**, and on Android on
at first only on a device without game controls of its own: "thor and desktop never needs it, only the fold does", and for the games where a tap cannot pick an answer "can a lightweight controller over lay be build instead ... just buttons that absolutely need to be used" (the maintainer, 2026-10-16)), for the games whose
script says **`-- reader: touch press`** among its first lines. In such a game a click or a tap
on the game's picture beside any text, or in a message's box beside its words, presses ○ (the
game goes on); a click on an answer of a choice moves the game's cursor there with the d-pad, one
press every quarter of a second while the script reports where the cursor is, then presses ○; a
press held half a second on an answer looks its word up instead; with the popup open a click
only closes it. While a choice is up a click beside it does nothing (○ would confirm whatever is
lit). If two presses of the d-pad leave the cursor where it was, something else has the input
(a message over the choice): ○ is pressed once and the picking goes on, 48 presses at most.
A game whose script says nothing is not played by touch: its clicks are as they always were
(the role-playing games, where a stray ○ is not wanted). `-- reader: touch game` is the
siblings' word for a game that takes touches itself; no PS2 game does. Words are still looked up
with a click on them outside a choice, and by pointing (Shift) as before.

**The script's side**: `reader.cursor(index [, "horizontal"])` says which answer of the choice on
the screen the game's cursor is on, from 0, and -1 while it is on none yet; `reader.cursor(nil)`
once the choice has gone (a script loaded anew starts with none). "horizontal" is for answers
side by side (left and right move the cursor). A script that reports choices but not the cursor
gets a message instead of a pick ("its script does not say where the cursor is"). A script calls
it as `if reader.cursor then reader.cursor(at) end`, so an older app runs it as before (no new
API version). `Engine::Cursor`, `LuaApi` `ReaderCursor`; the development channel's `cursor`
says what the script reports. `reader/qt/reader_controller.cpp` (`PlayingByPresses`, `GoOn`,
`PickAnswer`, `PickStep`; the PPSSPP Reader fork's, with the PS2's pad numbers) does it here.

**The games** (each cursor found in the game's memory in the harness, below): Never7, Ever17,
Remember11, CLANNAD and アマガミ report the cursor; in all five ○ picks nothing until the d-pad
has put the cursor on an answer (the first press puts it on the first answer). ひぐらしのなく頃に祭 and
EVE burst error PLUS say `touch press` too: a tap goes on, but their choices are not reported
(see each under "Games"; EVE's command menus are, as a `menu`), so their scripts also say
**`-- reader: buttons ...`**: the buttons ARMSX2 Reader shows as a column of round buttons at the screen's right
edge while Play by touch is on (▲ ▼ for a choice or a command list, ○, × to back out, △ the
game's menu: EVE's system menu, ひぐらし's message browser; each found in the harness, 2026-10-16).
The five that report their cursor name buttons too (2026-10-16, after ARMSX2 Reader on a phone stopped at Never7's title): each title wants START, which ○ is not, and each has its save and load menu on a button of its own: Never7 and Ever17 `start select up down circle cross`, Remember11 and CLANNAD `start square up down circle cross`, アマガミ `start triangle up down circle cross`, with ○ in every column (EVE's is `circle cross triangle`): on a phone, at Never7's LOAD MENU, × only backed out and nothing chose (each script's notes say what its other buttons do). The desktop app does not draw them. うたわれるもの was taken out (its battles are long play, not
reading). Not the role-playing and action games.

**Checked** (2026-10-16): tests with made-up memory for each script's rules and the API
(`reader/desktop/tests/touch_play_test.cpp`, `[touch]`), the catalogue's line and the setting
(`catalog.cpp`). The app on Linux, drawn in software, by the development channel's `click`,
`press` and `release` on the game's surface (where a real pointer's events arrive): in Never7
and CLANNAD at a choice with no answer lit, a click beside the answers changed nothing, a click
on the second answer picked it (Never7's answer went on to its own line; CLANNAD's story
variable F1089, where the game keeps the answer picked, became 1), clicks on the picture went
on through the messages after it; in Never7 a press held 0.8 s on an answer opened its word's
entry and picked nothing. The same press sequence as the apps' (`private/touch/pick.sh` in the
maintainer's checkout: the cursor read, a press of the d-pad, 250 ms, again) picked each
answer of a choice of two in Never7, Ever17 and CLANNAD and of three in Remember11, each going
on with that answer's own text; in アマガミ's first choice (three answers, reached from a new
game) the cursor followed the d-pad and a click on the first answer picked it. **Not tried**: Windows, a real touch screen (Qt hands a touch
to the window as a mouse press, which is what was sent), the other four games' taps in play.

## The Steam Controller without Steam (Windows)

To Windows the Steam Controller (2026) is no gamepad by itself: left alone it plays keyboard
and mouse. While Steam is not running, `reader/qt/reader_steam` reads the controller's own reports,
switches that emulation off, and offers a gamepad the emulator finds like any other (made
inside the program through SDL's virtual joystick: no driver), the right pad as the mouse
pointer (a trackball; pressed, a click), the left pad as the wheel (pressed, the right
button). `reader/desktop/steam_bridge` has what the reports mean and is the Batocera box's bridge
(`sc_bridge.py`) in C++. The grips stay the reader's (above). With Steam running nothing is
touched. The development channel's `steam-report HEX` plays a report on any system.

Checked (2026-10-10): tests of the reports' meaning with made-up reports; on Linux without a
display a made-up report made the gamepad in the running app. **Never run**: everything
that is Windows only (finding and reading the controller, switching its emulation off, the
mouse), and whether this emulator takes the new gamepad for player one without being told.

## The CRT picture

*Reader settings, CRT picture* shows the game as a cathode-ray tube would: scan lines, a
phosphor pattern, bulging glass, glow, a warm white, each with a slider, and six ready-made
looks. **On from the first start here** (a strong picture; the switch turns it off). It is one shader and one table of sliders that every reader
fork carries (`reader/core/crt.*`, `crt_shader.glsl`); both settings pages, on a computer and
on Android, are built from the table. **`docs/reader/crt.md` has all of it**: what each
slider does, how a slider is added, where it is tied to this emulator, and what was checked
how. The shared files are changed in the Flycast Reader fork and copied here by its `tools/reader/crt.py sync`: do not edit them here. Upstream files touched for it: `pcsx2/GS/Renderers/Common/GSDevice.*` and `GSRenderer.cpp`, each renderer's device file (and the Vulkan and Direct3D 12 headers' size of the push constants), and the constants' blocks in `bin/resources/shaders/vulkan/present.glsl` and `dx11/present.fx`.

## Game scripts

A game's text is read by its **game script**, the profile of the sections above: one Lua file
per disc, named after its serial. A published build of the app comes with none
(`-DREADER_BUNDLE_PROFILES=OFF`); a build of one's own builds `reader/profiles` in, as before.
Either way the scripts the app uses can come from **the published list**:
`reader/profiles/index.json` in the public repository (made with the scripts by
`tools/reader/scripts-index.py`), which **Reader, Game scripts** (System Configuration, *Game
Scripts*) fetches with the emulator's own downloader (`READER_SCRIPTS_URL` names another list,
also a file on the disk). Each game of the list or of the app is a row: *Not installed*,
*Installed*, *Update available* (the list has another script than the one installed from it,
or than the one built in), *Your own script* (one the list did not give, or one of its that was
changed since: installing the list's asks first), *Needs a newer app* (`-- reader: api N`
above what `Scripts::Api` is). *Install* fetches the script and any data file it names from
beside the list and checks each against the list's size and SHA-256 before anything is
written; *Remove* takes the script and what came with it. *Install from a file…* takes
`<SERIAL>.lua` and `<SERIAL>.<name>` files of the user's own. All of it is in the user's
`reader/profiles` folder, which wins over a script built in, as it always did; the folder's
`.installed.json` says what came from the list. A script installed or removed while its game
runs is put to use at once.

**What is in the list.** Every script whose data files are beside it in the folder.
`scripts-index.py` leaves out a script that asks for a data file the folder does not have:
in the public repository, where a character table made from a game is not, that is the two
ペルソナ, 真・女神転生III and DIGITAL DEVIL SAGA. The list is made when the public repository
is (a maintainer's step); it is not kept in Git here.

**The format** (`format` 1): `{"format": 1, "system": "ps2", "scripts": [{"id", "title",
"reads", "version", "api", "file", "size", "sha256", "data": [{"file", "size",
"sha256"}]}]}`. ARMSX2 Reader reads the same list from the same address (its `ScriptStore.kt`
is this logic in Kotlin): a change to the format goes to both, and to `scripts-index.py`.

**A script for a new game** is written as each game's above was: the game run without a
display (`screen.sh`), its memory looked at (`dc.py dump`), the profile tried as it grows
(`dc.py "profile PATH"`), its reports read against pictures of the game. It holds addresses
and decoding logic only. It begins with the lines the app shows (`-- reader: title`, `reads`,
`version`; `no hooks` for one that reads memory each frame, as all here do; `touch press` for a
visual novel played by touch, "Playing by touch") and refuses any
other program than its disc's (`reader.build_id`).

Checked (2026-10-14): `[scripts]` tests with made-up scripts (the list read, the states, a
file that is not the list's refused, installing, updating, removing, files of the user's own);
the app on Linux without a display against a list served on a loopback port: the window and
its states, *Update* fetching the script over HTTP, the user's change seen as their own,
*Remove* with its question, and a build without scripts going from none to two installed.
**Not tried**: *Install from a file…* in the window (its logic is tested), a script installed
while its game runs, the published list on GitHub, anything on Windows.

## Games

A profile holds addresses and decoding logic for one disc and refuses any other program
(`reader.build_id` is the program's CRC, as the emulator's title bar shows it).

**ひぐらしのなく頃に祭 カケラ遊び** (`SLPM-66913.lua`, program C08A36F0, disc version 1.01).
The game keeps its script in memory whole and lays a message out once, into a list with a
place and a moment for every character (in a scene object the game allocates, which the profile
finds through the program's pointer to it: its place moves between runs, by 16 bytes once
autosave is switched on, 2026-10-14); the profile reads the message being shown and that
list, and reports each part between two waits when it has come up in full. The file's
comments have the format. Seen in the first chapter's opening: narration, lines with a
speaker, a message in several parts, words with furigana, the cells on their characters.
**Not looked at**: choices, the TIPS, the backlog and the menus of the game (their text is
not read), whether a part is still reported while the text window is hidden or the game
skips, the other chapters, a savestate loaded in the middle of a message.
**Touch play** (2026-10-16): the script says `touch press`, so a tap beside the text goes on
(○); its choices are not reported, so an answer is picked with the controller. Not tried in play.

**Remember11 -the age of infinity-** (`SLPM-65550.lua`, program B33522CD, disc version 1.02)
and **Ever17 -the out of infinity- Premium Edition** (`SLPM-65421.lua`, program C577D399,
disc version 1.01). One engine (KID's), so one profile twice with each program's addresses;
a change to one belongs in the other. When a message begins the game puts its text, the
script's percent commands taken out, into a buffer, and fills a table with one entry per
character: how far from the text window's left edge it stands, its glyph's columns, how
visible it is. A row has no number; a new one begins where a character stands no further
right than the one before. A speaker's name is the text before the opening 「 (the game
shows it in a box of its own). A choice's answers are in buffers of their own without a
table, so the profile lays each row out itself from the font's table of glyph columns, centred
where the game centres them (measured in pictures). Seen in each game from a new game through
its first choice and a little beyond: narration, spoken lines, a message of two rows, the
choice with the message still up, the cells on their characters. In the harness a choice
takes the cursor being moved before ○ does anything.
**Not looked at**: a message that goes on after a wait in the same window (written for,
never met), a line break inside a message, coloured or large text, the TIPS and the
shortcut/menu screens (not read), choices with more rows than three or a time limit,
characters from the second half of Shift-JIS in a choice (their columns are guessed at),
what happens while the game skips, a loaded save or savestate, anything past the opening.
**Touch play** (2026-10-16, "Playing by touch"): the cursor is a byte a little before the
answers' count (Ever17 0xC2B572, Remember11 0xB17DCA; the count of the answers drawn is the byte
after it): 0xFF until the d-pad is first pressed, then the answer from 0, kept after the choice.
Seen in each game's first choice from a new game (two answers in Ever17, three in Remember11):
the byte following the d-pad, both ways round, and each answer picked by the apps' presses.

**Never7 -the end of infinity-** (`SLPS-25256.lua`, program 9D260CAF, disc version 1.01).
An older engine of the same house, and nothing like the two above. The game keeps the rows it
has shown, as it broke them, in a list (a row is added when it has been typed out; a
speaker's name is a row of its own that begins with a space), and a number says which row of
the text window the last one stands in. It has no table of places: characters are drawn 24
apart, brackets and commas 12, which the profile repeats (measured in pictures). Whether a
row goes on in the next or ends there is not kept either: a row that ends in a full stop, a
closing bracket or the like is taken to end. A pause in the middle of a row is reported as
two parts. A choice's answers are in buffers that keep them afterwards (an earlier choice's further
answers too), so what says when one is up is the place of the choice's box, and a count how
many answers it has. **The list holds 999 rows**: when it is full (some time into the first
morning) the count stays and every row moves up a place for each new one; the profile finds
the rows it reported last again where they moved to. Only the rows the window shows are
reported (rows that went by while the game skipped are not), and while a choice is up the
message under it counts as typed out. Seen from a new game through the first choice and some
forty messages beyond: narration, spoken lines with the name in its box, a window of four
rows, the pause, the choice, the cells on their characters. The game starts in its auto
mode in the harness (△ leaves it).
Since 2026-10-14 (while going on to the next choice, above): the list full and moving, the
first days to Yuka's tennis and the evening after, choices of two, three and four answers
(the height of three and four is guessed from two; the cells were not looked at).
**The page** (2026-10-16): some narration fills the screen instead of the box (the
earthquake at 11:17 on the 4th, for one). The rows' pitch at 0x3A08D0 tells the two apart (28
in the box, 29 on the page); the page's rows begin at 81 (measured) and it holds 12. Until then
the profile put the page's earlier messages below the picture and only the last could be
pointed at. The page also breaks rows its own way: the list keeps the box's rows, but the page
draws at most 24 characters to a row whatever their width and gives the rest to the next row,
with a character that may not begin a row (』, ー, small kana...) taking the one before it
along. Seen on Linux from a save of the maintainer's (4/4, 10:49) through the earthquake page
and the pages after it: every message of a page reported, the cells on their characters
(also the rows broken again), a word in an earlier message of the page clicked and looked up,
the box after the page as before. **Not looked at**: a message whose last row gives characters
away (drawn here on a row of its own below; never met), a pause in the middle of a row on the
page, a speaker on the page.
**Not looked at**: other half-width characters than those met, the game's own skipping, the
append scenarios.
**Touch play** (2026-10-16, "Playing by touch"): the cursor is the 16-bit number at 0x53D896,
the answer from 1, 0 until the d-pad is first pressed at a choice (the game sets 0 for each), and
the answers' count is at 0x53D890. Seen from a new game through fourteen choices (two, three and
four answers: the tennis and the drinks) and at three later ones from a save (4/4): the number
following the d-pad both ways; picked by clicks and by the apps' presses. The choice's box word
(0x397F4C) stays set through the credits after an ending, so the last choice's answers still
count as up there (a tap on one presses the d-pad and ○ in the credits, harmless; not mended).

**ペルソナ4** (`SLPM-66978.lua`, program 4E63E63C). The first game here that is not a visual
novel. Its messages are Atlus's message files (commands that begin with a byte from 0xf0 on,
characters as one byte or as two that number a cell of the game's font), which the profile
does not read: when a page comes up the game lays it out into rows of characters, every
command carried out and every name put in, and the profile reads those rows from the
window that shows them (64 places for windows; a window says whether its box is up, whether
the page is still being typed and whether a choice is up). A row has where it begins and how
wide it is, a character how far it is to the next, so the cells are the game's own. Rows of a
message are one text; the `>` that begins what is told and the space a later row is set in by
are left out. The speaker is the name the message file gives the message.
**The font's cells are in an order of the game's own, so the profile needs a table**
(`SLPM-66978.charset`: UTF-8, the characters of cell 0x20 onwards in the font's order, a line
break after every 128; not in Git, see "Ground rules"). The one used here is the Persona 4
table of the public Atlus-Script-Tools (`Charsets/P4.tsv`) in that form; every message in
memory at four moments (some 4,800 pages) came out as sensible Japanese with it, and what
the reader reported was read against pictures. It gives a few marks in their plain form
where the game draws a wide one (`?`, `"`).
Seen from a new game to the first evening at the Dojimas': the questions before the story
with their choices, the Velvet Room, the arrival, the petrol station, the house (some 90
pages): told and spoken text, names (also one with a space), choices of two, three and four
answers with the message still up, the cells on their characters, a window whose box had
gone and came back; one scene's reported lines against the message file's, none missing.
**Not read**: everything that is not a message window (the menus, the date, battles' own
texts, the name entry, films). **Not looked at**: a battle, the menus' help texts (which
may or may not be message windows), coloured text, text that waits in the middle of a page
(reported when the page is whole), a page that goes on by itself (written for, never met),
two windows at once, a loaded save or savestate, skipping, anything past the first evening.

**ペルソナ3** (`SLPM-66445.lua`, program 05C3D28F). ペルソナ4's engine a version older, and
its profile with other places: the table of windows is at another address with places of 52
bytes, and a window's message, rows and answers are 16 bytes further on. All that is said of
ペルソナ4 holds. Its table (`SLPM-66445.charset`) is the Persona 3 table of the public
Atlus-Script-Tools (`Charsets/P3.tsv`) in the same form.
Seen from a new game to the dormitory's hall: the questions about the difficulty with their
choices, the name's confirmation, the arrival (some 20 pages): told and spoken text, the
speaker's name, choices of two and three answers, the cells on their characters.
**Not read**: what is not a message window (menus, the name entry, battles' own texts, films
and their subtitles if any). **Not looked at**: anything past the first night, a battle, a
loaded save.

**ドラゴンクエストV 天空の花嫁** (`SLPM-65555.lua`, program B64E9939). The message window
keeps its rows as one Shift-JIS text with line feeds, everything put in, and beside it where
the message's source is (nothing while no window is up), how many characters the rows have,
how many are typed (a float) and whether the rows are moving up. When the message goes on,
the rows move up and new ones are typed below those that stay: the profile reports the new
rows when they are all there, a part for each speaker (a row that begins with a name and 「).
The name is the speaker; `＊「` is somebody unnamed. A row has no table of places: the game
draws from the window's left edge with a font whose every character has a width, whose
table is in memory ("FONT"; code, width, picture), in a picture 512 wide, and the profile
does the same (a space is narrower than the table says: measured).
Seen from a new adventure log through the birth and the ship's cabin (some 20 messages):
speakers, messages of two and three rows, rows that stay while new ones come, the cells on
their characters; the messages about the memory card before it.
**Not read**: menus, the battle's window, choices (はい/いいえ are a menu), the name entry.
**Not looked at**: a battle (whose messages may or may not be this window), a message with
a number or an item's name put in, the fast message speeds, a loaded save.

**サイレントヒル2** (`SLPM-65051.lua`, program E36E16C9; Japanese text only). A message is a
row of 16-bit numbers: below 0xf000 a character, which is the place of the character in a
font that has the characters of Shift-JIS in their order without the gaps after 223 places
of its own (the profile has the runs of codes that are characters and needs no table);
0xfffd begins a new row, 0xffff ends the message. The game keeps where the message on the
screen begins, how many characters are shown (nothing while no text is up) and where the
rows begin from the left; characters are 27 apart and rows 30, around a line near the
bottom, in a picture 640 by 448 (measured). The rows of a message are one text.
Seen through the opening: the first words at the mirror and the whole of the letter's scene
at the car (some 17 messages of one and two rows), the cells on their characters.
**Not looked at**: what is told when something is examined (the same text system by the
message files, but none was reached: the car park has nothing), memos and the map's notes,
the menus, a choice (はい/いいえ), the save screen, the English text setting (the profile
would report nonsense for it: its numbers are the font's own first places).

**真・女神転生III NOCTURNE マニアクス** (`SLPM-65462.lua`, program 0E762E8D) and **DIGITAL
DEVIL SAGA アバタール・チューナー** (`SLPM-65597.lua`, program 44865CE1). One engine, so
one profile twice with each program's places; a change to one belongs in the other. The
text is Atlus's message format, but nothing of a message file is read: whatever the game
shows it lays out into rows of characters first, kept in a pool of 513 places (a row: where
it begins, its width, its characters as a list with each one's step, the next row, the
first). A place not in use is in a list of free places and keeps what it had, so only
places without a next one are read. The game lays some texts out twice and some anew every
frame in other places, and some move: a text is known by what it says and where, is
reported when its rows have stayed the same for six frames, and is not reported again when
the same characters were on the screen a moment before. A message window's name is a text
of its own above the message and becomes its speaker.
**Two fonts, two tables.** A row is in the large font (what is said) or the small one
(everything else: names of places, menus' help, prompts), by a bit of its first number, and
each font has its cells in an order of its own. No public table was found, so the tables
(`SLPM-65462.charset`, `SLPM-65597.charset`: the large font's characters, an empty line, the
small font's) were made here from the fonts in memory: the signs and kana are in ペルソナ3's
order; the kanji (1,929 and 2,028 in the large fonts, which are the same up to cell 2347)
were matched to a Mincho font's and then every one read by eye on sheets, 143 of the
matcher's readings corrected; the small fonts (1,030 and 1,241 cells, the same up to cell
996) are the large font's characters in several runs of its order, lined up by shape and
their last third read by eye. **A wrong kanji here and there is likely**: a cell misread
shows as a wrong character in a sentence, and the table is where to mend it.
Seen in 真・女神転生III from a new game to the station (some 45 messages: the voice, the
train, the mail), and in DIGITAL DEVIL SAGA through the opening's scenes into the base
(some 40 lines said in scenes, six message windows with their speakers): the text against
pictures, the cells on their characters, the menus' help and a place's name in the small
font.
**Not right**: a choice's answers come out as one text (はいいいえ); a name the player gave
has no code and is left out of what is said; the notice that a scene cannot be skipped is
left out by what it says (its rows are there for the whole scene, shown or not, and nothing
was found to tell). **Not read**: tables of characters (the name entry). **Not looked at**:
a battle, the menus proper (items, skills, the fusion screens), shops, anything past the
places named, a loaded save.

**アマガミ** (`SLPS-25918.lua`, program D72C9E2A). Plain Shift-JIS, no table, and nothing
like the others: no place was found that says which message is up or how far it is typed.
Two things are read instead. The scene's script is in memory as a row of values, its
messages among them as texts (16-bit numbers, a Shift-JIS character's bytes the other way
round; `／` begins a row, `W15` waits, `V1869` is the voice, `R word R 2 reading` is
furigana). And the game's list of things to draw has a picture for every character on the
screen with its place, though not which character it is. So when the pictures in the
window have stopped coming, every text of the scene is tried against them (as many
characters, rows breaking where its rows break, the same picture wherever a character is
the same), and which picture is which character is learnt from every message made out,
which soon leaves one text only. Before anything is learnt (after a start, or a loaded
state), a message is taken when it has stood for a second and a half, is at least eight
characters and is the only text that fits: **the first short messages after a start go
unreported**. The name in front of a message is its speaker once its characters are
learnt. The cells are the game's own places.
Seen from a new game through the prologue, the morning at home, the way to school and the
classroom (some 150 messages): thoughts and speech with and without a voice, messages of
one to three rows, a window that still has the message before, speakers, the cells on
their characters. A choice of three answers and a word with furigana were read by an
earlier form of the profile (which followed the script's own position; that works only for
lines without a voice); **the present form has met neither**.
**Not read**: the map and its events' names, menus, the name entry, the conversation
game's topics. **Not looked at**: anything past the first classroom scene, a loaded save,
skipping, what the profile costs on a slow device (it reads some 250 KB of memory every
fourth frame while a message is up).
**Touch play** (2026-10-16, "Playing by touch"): the game keeps no number for the cursor that
could be found (only in short-lived objects), so the script looks at what is drawn: the bar that
lights an answer is a plain item of the draw list, 560 by 36 in the picture, over the answer's
row, and there is none until the d-pad is first pressed (○ picks nothing then). While a choice is
reported the script looks for that item every fourth frame and says which answer's row is under
it. Seen at the first choice of a new game (three answers, the evening at the hill's park): the
answer following the d-pad, none before; a click on the first answer picked it (the next message
was its thought). In the harness a choice is not made out after a save state is loaded at it (as
before: nothing is learnt yet and the answers are short), so it was reached from a new game.

**EVE burst error PLUS** (`SLPM-65320.lua`, program 8EC6655D). Plain Shift-JIS. The game
copies a message into a buffer of its own (its parts with a NUL between them; an `s` before
the NUL is the script's wait) and puts what the window shows into a text object: how many
characters are up and the characters, one 32-bit number each (the code's bytes the other way
round). A word beside it is 1 while the window waits for the button, but only once a voice is
over, so a part is also taken when as many characters are up as the message has to a part's
end. The speaker is the name in 【】 at the message's front. The object has no places: the
profile sets 23 characters to a row, later rows in by one, with the game's breaking rules (a
、 or 。 that would be the 24th hangs out of the row; ‥, closing brackets and small kana do
not begin one), all measured in pictures.
Seen from a new game through the office and Akane's visit (some 90 messages): narration,
speakers, voiced lines, messages in parts, the cells on their characters.
**Not looked at**: the investigation's command menus (見る, 話す...) and choices, the second
scenario (まりな), a loaded save, skipping.
**Touch play** (2026-10-16): the script says `touch press`: a tap beside the text presses ○.
**The command menus** (見る・調べる, 移動する, 話す and what each opens) are reported on the
channel `menu` (the apps let a tap pick an entry but do not read it: no statistics, no backlog,
no panel): the game runs a menu in a routine whose stack frame is always at the same place, with
the count at 0x1FFE900, the entry lit at 0x1FFE920, the entries as pointers to the scene script's
strings from 0x1FFEBF0 (each beginning with a full-width space) and the level at 0x1FFEA70 (0
the first, 1 what an entry opens). The frame stays after the menu closes, so a menu counts as up
only while the list's end (0x1FFE910) is where the count says and every entry is such a string.
Places measured in pictures: the first level's rows from (53.8, 93.5) in the 640x448 picture, 24.2
apart, 17.9 a character; the second level 60.4 right and 66.5 down; a third is guessed. Seen on
Linux: the menus of the office and their second level reported, the cursor following the d-pad,
the cells on the entries, a click on 移動する opening 外に出る with nothing counted. The script's
buttons are now × (back out of a menu) and △ (the game's menu). Its story choices were not met.

**うたわれるもの 散りゆく者への子守唄** (`SLPS-25679.lua`, program BF9554BE). Plain
Shift-JIS. When a message comes up the game lays it out once into a list, 24 bytes a
character: its code, where it stands in the box, how wide it is; so the cells are the game's
own. The speaker's name is a string of its own (empty while nobody is named). A reading is
written `[word*reading]` in the script, which the game keeps beside the list: the profile
takes the readings from there. The game's long dash (its own code 0x875B) is reported as ―.
A message is reported when its list has stood for four frames.
Seen from a new game through the prologue and the first scenes with Eruruu (some 45
messages): told and spoken text, speakers, a reading over a kanji, messages of two rows, the
cells on their characters.
**Not looked at**: the strategy battles and their text, choices, the menus, a loaded save,
skipping.
**Touch play** (2026-10-16): the script says `touch press`: a tap beside the text presses ○,
also in a battle (where ○ carries out what is lit); choices are not reported. Not tried in play.

**シャドウハーツ** (`SLPS-25041.lua`, program 3E46A5F0). Plain Shift-JIS with commands of
three bytes that begin with `%`. Every window is an object kept by a window keeper; the
keepers are a chain that begins at a fixed place, and a keeper of message windows is told by
its first word. Each row of a message is a window of its own, linked to the next, with how
far below the first it stands, its text, a list of where each character stands and a state
that says whether the row is still being typed. The keeper has where the window is. A
speaker's name is a window whose text begins with `%t`. A message is reported when all its
rows are typed and nothing has changed for ten frames (the next row begins two frames after
one is done); one that goes on after a pause reports only what is new.
Seen from a new game through the train, the fight on its roof and the woods after it (some
45 messages): speakers, messages of one to three rows, the cells on their characters.
**Not read**: battles' help and menus (other keepers), the opening film's captions (part of
the film). **Not looked at**: choices, shops, a loaded save.

**METAL GEAR SOLID 2 SUBSTANCE** (`SLPM-67002.lua`, program CED0851A, disc version 1.01;
Japanese subtitles). The first game here whose kanji have no codes at all. Text is two bytes a
character: `80 xx` ASCII, `81 xx` hiragana and `82 xx` katakana in Unicode's order without
ゐゑ/ヰヱ, `83 xx` signs of the game's own table (○△□× are the buttons); the 0x40 and 0x20 bits
of the first byte only change the spacing. Each text resource (a codec conversation, a
cutscene's or radio call's subtitles) brings the pictures of the kanji it uses (24 by 24, two
bits a pixel, 144 bytes), and a kanji is the number of its picture there, 255 to a first
byte (`8C`-`8F` in the codec, `90`-`93` in subtitles). So a kanji is known by its picture:
**the profile needs a table** (`SLPM-67002.charset`: a hash of each picture and its
character; not in Git, see "Ground rules"), made here from every picture in the game's files.
The pictures are the same wherever a kanji is used, so 1,453 cover the game: those of the
codec (CODEC.DAT, the second layer of the disc: a dual-layer image has a second directory at
the layer's start, which the ISO's first one does not list), the cutscenes (DEMO.DAT,
DEMO2.DAT), the radio calls (VOX.DAT, VOX2.DAT) and the films (MOVIE.DAT, MOVIEVR.DAT). They
were matched against Noto Sans CJK Bold and MS Gothic and **every one then read by eye** on
sheets and checked in its sentences (some 200 of the matcher's readings corrected); the
whole script read back as sensible Japanese (6,139 different lines). `#{base、gloss}#`
writes a gloss over a word (ペンタゴン over 国防総省): the base is what is read, a gloss in
hiragana is its reading, a button's picture is read as its gloss (アクションボタン).
Where the line on the screen is: the **codec** draws a line once, as the script command that
shows it runs; that command's arguments (a pointer at 0x1C955C) are a tag, 0x4AB569, and the
line, which must lie among the texts of the codec's block (header at 0x1C9288). **Subtitles**
are an object with a list of entries (start, end, length, text) and the pictures after them;
the pictures in use are at 0x191384, and the entry up is the one whose start and end hold
the object's clock. The cells follow the game's own drawing (measured): 24 a character
(ASCII from the game's table of widths), rows left-aligned in a block centred at 320, in
which a row that ends in a line break counts 22 wider (so the codec's lines, which always end
in one, centre at 309), the codec's rows from 263 every 33.75, subtitles' from 369 every 36.
Subtitle rows have no punctuation between them, so they are joined with a space.
Seen on Linux from a new game (Tanker) through the opening and the first codec calls on the
deck (some 60 lines): the bridge and the deck's cutscenes, the radio conversation at the
start, a call to Otacon (141.12) on the deck, glosses, a button in a line, the cells on
their characters (a word pointed at was looked up, in the codec and in a cutscene).
**Not read**: the menus, items' names and help, the briefing files, the dog tags, anything
drawn as a picture (the help of the title's menus: their kanji are not in these fonts).
**Not looked at**: the Plant chapter, Snake Tales (their text may be elsewhere), the codec's
save conversations, a subtitle with three rows, the English text settings.

**METAL GEAR SOLID 3 SNAKE EATER** (`SLPM-65790.lua`, program 0481AD8A). METAL GEAR SOLID
2's text and drawing (read its paragraph), at this program's places; a change to one profile
belongs in the other. The game's code is unpacked by a loader, so its places were found in
memory (the glyph lookup at 0x11E1F0 gives the font table at 0x1D30E0, as 0x1166A8 does in
METAL GEAR SOLID 2). Differences: the codec's command arguments are at 0x498CE0 (tag
0x61D3F4), its block at 0x213A80; a subtitle object begins with 4, has its entries' length
at +0x28 and the entries from +0x2C, an entry's length in the low half of its fourth word;
the codec's rows start at 274.5, every 34. **Its table** (`SLPM-65790.charset`, not in Git):
1,540 pictures, 1,218 of them the same as METAL GEAR SOLID 2's (the same font and drawing,
bit for bit), the other 322 matched and read by eye on sheets, most checked in their
sentences. **On the disc the codec's texts are enciphered** (a block's fifth word is
0x41915525 until the game deciphers it in memory): the key stream is the same for every
block, so the one met in memory deciphers 903 of the 963 banks offline; the 60 longest are
not deciphered here (their pictures are plain, so their kanji are in the table, but three of
them were read without a sentence to check them in: 慄, 膚, 禽). Nothing of this matters
to the profile, which reads the deciphered text in memory.
Seen on Linux from a new game: the opening's subtitles (the briefing in the plane, some 15
lines), the first codec call after the landing (its lines with a gloss, 『』), the cells on
their characters (a word pointed at was looked up, in the codec and in a subtitle).
**Not read**: the menus (their help is in this encoding too and could be read: not done),
the survival viewer, the camouflage index, item names and help. **Not looked at**: a
subtitle of three rows, the radio calls while playing (VOX: the same object as subtitles),
anything past the first codec call, the 60 large codec banks in play.

**CLANNAD** (`SLPM-66302.lua`, program 7800DC84, disc version 1.01; HuneX's engine, not
the PC's RealLive). VN Library's notes (`docs/formats-clannad-ps2.md` and the other
`clannad-*.md` in the decompile repository) have the engine's format and the addresses of its
state. The message is in one buffer (0x376F38) as the script has it: Shift-JIS, a speaker as
`【name】` in front (drawn without the brackets, the name first on the row), `＊Ａ`, `＊Ｂ`... for
the names the player gives (eight bytes each from 0x376CF8: 岡崎, 朋也 at first), `^` a line
break; when a message goes on after a wait, the buffer grows and only what is new is reported.
A message is reported when it has stood for four frames. Characters stand 23.6 apart, 23 to a
row, rows 28.2 apart from (34, 334) in a picture 640 by 448 (measured); which characters may
not begin a row is a guess (、。」 and small kana hang at the end). A choice (`SEL`): the bit 0x10
of 0x3775B8 is set while it waits, its answers are buffers of 100 bytes from 0x377290 and their
number is at 0x3775B0; they stand in the message window's rows. A `SEB` choice's answers are
digits that pick texts from a table through story variables (the game's code at 0x14B344),
which the profile does likewise (written, never met). In this game ○ picks the answer under the
cursor at once, but going on to the next choice stopped with the choice up and nothing picked.
**Routes** ("Routes finished elsewhere"): the global variables G (16 bits from 0x372F08) are
the game's progress, kept in its system data: each route's ending sets a cell (VN Library's
`clannad-menu.md` has which, from the script's own assignments: 渚 1, 風子 2, 智代 3, 杏 4,
ことみ 5, 有紀寧 6 and its light 30, 春原 9, 勝平 12, 美佐枝 13, 幸村 15, 美佐枝's light in
智代's route 31; AFTER STORY's lights 7, 8, 10, 11, 14, 33, its first ending 35, the true one
45), G0 counts the lights and G73 is the title's stage (2 opens AFTER STORY). 椋's route sets
no cell of its own and is not offered (AFTER STORY does not need it). Marking one sets its
cells, counts its lights and raises the stage as the game's title does.
Seen on Linux (2026-10-15): from a new game through the prologue, the walk to school and the
classroom to the first choice (the tape recorder) and past it: told and spoken text, speakers
(声, 教師, the player's name), a message that went on after a wait, a choice of two, the cells on
their characters; the next choice from the classroom to that choice in 18 seconds (170 texts),
stopped with the choice up and nothing picked. Then every School Life route marked through the
development channel, the system data saved from the game's own menu (□, システムセーブ) onto a
new card of its own, the emulator started anew: the game loaded its system data by itself and
its title showed AFTER STORY, CG MODE and MUSIC MODE and eight lights.
**Not looked at**: a SEB choice, a choice of three or more, a long message's row breaks (the
23 is from one message), a name the player changed, the game's own skipping, AFTER STORY,
the window on a computer that is not this one. In the harness, after a savestate is loaded the
game draws only some characters of its text (the profile reads them all the same); text drawn
straight over a picture (the prologue, the title's lights) has a black box behind it here,
with the hardware renderer (the software one does not start on this machine).
**Touch play** (2026-10-16, "Playing by touch"): the cursor is the 16-bit number at 0x60CDDA,
the answer from 0, 0xFFFF until the d-pad first lights one (○ picks nothing before; a press of ○
alone was tried). It lies in an object of the choice window's, not in the program's own data,
but was at the same place in a new game started anew and in a save state of the day before.
Seen at the first choice (the tape recorder): the number following the d-pad both ways, each
answer picked by clicks and by the apps' presses, the story variable F1089 holding the answer
picked; the next choice (謝っておく) began at 0xFFFF too. **Not looked at**: a choice of three or more, AFTER STORY.

## Windows

Upstream supports Microsoft's compiler (or clang-cl) on Windows and nothing else, with its
own package of prebuilt libraries; MinGW, which the siblings are cross-compiled with, is not
supported. The Windows app is therefore made on Linux with the `clang-cl` and `lld-link` of
the toolchain folder, against Microsoft's C runtime and Windows SDK.

```sh
tools/reader/build-windows.sh         # build-windows/pcsx2-qt/pcsx2-qt.exe
tools/reader/package-windows.sh       # build-windows/package/PCSX2Reader-windows.zip
```

What they need, under `~/.local/share/ps2-reader/windows` (`env.sh` has the names):

- `xwin-sdk`: Microsoft's C runtime and Windows SDK, fetched and laid out by the tool `xwin`
  (`xwin --accept-license splat --output …`). **They are Microsoft's and come under
  Microsoft's licence terms**, which whoever runs that command accepts; the maintainer did.
  Nothing of them is in the repository or in the package.
- `deps-x/deps`: upstream's package of Windows libraries (its `pcsx2-windows-dependencies`
  release), which has Qt 6.12.
- `qt-host`: a Qt of the same version for Linux, for the tools that run during the build
  (`moc`, `rcc`, `uic`, `lrelease`); conda-forge's `qt6-main`.

What the build does about being on Linux: `tools/reader/cmake/windows-toolchain.cmake` names
the compilers and the folders; `windows-case.py` makes links for the headers and libraries
the sources call by another letter case than Microsoft's files have; `cmake/llvm-ml64.sh`
stands in for Microsoft's assembler. The package is upstream's layout (the libraries beside
the program, Qt's plugins in `QtPlugins`, `resources`); `pe_check.py` checks that every
function a file of it imports from another is there. Microsoft's C++ runtime is not in the
package: a computer without the "Visual C++ Redistributable" is told so by the app (or by
Windows). The game patches upstream downloads into `resources` (`patches.zip`) are not in
it, and the libraries for video capture are a newer FFmpeg than this version of the
emulator looks for, so recording video will say it cannot.

**The Windows app has never been started**, here (no Wine on this machine, ever) or on a
Windows computer. That it links and that its imports are satisfied is all that is known.

## How it was checked

- **Logic**: 118 tests with made-up text (`tools/reader/test-desktop.sh`).
- **The app on Linux without a display** (2026-10-09): it starts into the PS2-style main
  menu; the keys move between its entries and into System Configuration and the Browser and
  back; System Configuration scrolls to its last line; the Browser with fourteen made-up
  games (covers and plain cases, the badge, the chosen game's name and serial, the second
  row following the choice); the reader's settings, dictionaries and statistics windows as
  pages of System Configuration. Looked at in pictures of each.
- **A game on Linux, drawn in software** (2026-10-09): ひぐらしのなく頃に祭 カケラ遊び
  from its first start to the first chapter's opening lines, the buttons pressed through
  `dc.py`; the text the profile reported compared with pictures of the game, and the cells
  drawn over a picture; the audio history holding sound at the title screen. Remember11 and
  Ever17 each from a new game through the first choice (some 60 and 230 messages), the
  reported text read against the pictures; Never7 the same way, some 130 rows of text.
- **ペルソナ4 on Linux, drawn in software** (2026-10-13): as its paragraph under "Games"
  says. The picture of a game for a card or for the harness (`grab`) is now taken on the
  emulator's own thread: taken beside it, it hung for good in this game's 3D scenes.
- **Five more games on Linux, drawn in software** (2026-10-14), and then EVE burst error
  PLUS, うたわれるもの and シャドウハーツ: as their paragraphs under "Games" say; each profile written against the running game, its reports read against
  pictures of the game and its cells drawn over one.
- **Not seen**: a word clicked in the game and looked up (no dictionary was installed in
  the test folder); a card; a real games folder in the Browser beyond the one game;
  starting a game from the menu; the mouse; how any of it moves (the pictures are stills);
  anything on Windows.
