# The CRT picture

*Reader settings, CRT picture* shows the game as a cathode-ray tube would show it. **In this app it is on from the first start** (the build says so: `READER_CRT_DEFAULT_ON` on the reader's core); the switch turns it off.
It is the reader's own: the emulator's own filters and shaders are
left as they are, and while the picture is on it takes the place of the TV shader.

## What it does

One switch, six ready-made looks, and a slider for every part. A change shows in the game at
once.

| Group | Sliders | |
| --- | --- | --- |
| Lines | Scan lines, Beam, Lines, Sharpness | The dark gaps between the tube's lines; how much a bright line swells and closes them; how many lines there are; how small the beam's spot is along a line. |
| Phosphor | Phosphor, Phosphor strength, Phosphor size | The pattern of red, green and blue the tube's face is made of: aperture grille (stripes), slot mask (bricks), shadow mask (dots), or a fine one for small screens. |
| Glass | Warp, Corners, Vignette, Reflection | The bulge of the glass, its rounded corners, the darker corners, a lamp mirrored faintly in it. |
| Light | Glow, Halation, Bloom, Brightness | Light that spreads a little around what is bright, light that spreads far and warm through the glass, bright light running over the phosphor's pattern. |
| Colour | Warmth, Colour, Contrast | The tube's white from bluish to amber, the strength of its colours. |
| Signal | Convergence, Grain, Flicker | Red and blue landing apart, snow, a hum bar wandering up the picture and lines that tremble. |

The looks: **Old tube, strong** (the values a new install has, which the maintainer chose on
their own screen: 240 thick lines, a plain slot mask, a strong bulge, much glow, a little
snow and flicker), **Warm anime, soft** (the gentle one, with the console's own number of
lines), **Trinitron**, **Old television**, **Arcade cabinet**, **Studio monitor** and
**Scan lines only**. Moving a slider makes the picture "My own".

Things worth knowing:

- **Lines: as the console** takes the console's own number of lines (448 for a PlayStation 2) and halves it
  until the screen has three of its pixels for each: fewer cannot show a line and its gap,
  and lines that are still too fine fade out rather than beat against the screen's pixels.
  A number chosen by hand is taken as it is.
- **A picture drawn finer than the console drew it** (a higher rendering resolution) keeps
  its detail: the lines are laid over it. A picture at the console's own resolution is
  shown as its rows, each the middle of a line.
- **Phosphor size: as the screen** is one pixel up to 1080 lines of screen, two at 2160.
- **The words stay where the text is.** What the reader lays over the game's text (the
  places a click or a tap finds a word by, the mark on a word) is bent with the glass.
- **The handheld make is lighter**: the Android apps build the shader with fewer samples for
  the glow and along a line. Everything else is the same.

## How it is made

One shader, one pass, the same text in every reader fork and for every graphics API:
`reader/core/crt_shader.glsl`, written in what GLSL ES 1.00, GLSL 3.30, Vulkan GLSL and HLSL have
in common. It was written here; what it does follows the CRT filters of
[michadame](https://github.com/Morgawr/michadame) (its two tube filters, its glass and its
cathode interference), which the maintainer asked to be the model, and its phosphor patterns
follow Timothy Lottes' public-domain CRT shader, as michadame's do.

| File | |
| --- | --- |
| `reader/core/crt.h`, `crt.cpp` | The table of what can be set (`kParams`), the looks (`kPresets`), the values, and `Pack`: the eight vectors (128 bytes) a renderer hands the shader. Knows nothing of an emulator or a toolkit. |
| `reader/core/crt_shader.glsl`, `crt_shader.inc` | The shader, and the same as a C++ string (made by `crt.py`). |
| `reader/qt/reader_crt_page.*` | The settings page on a computer, built from the table. |
| `reader/core/crt_jni.h`, `(ARMSX2 Reader) reader/ui/CrtSettings.kt` | The Android apps' bridge and their settings page, built from the same table. |
| `reader/desktop/tests/crt_test.cpp` | Tests of the table, the kept text, the looks, what the shader is handed, the glass's bend. |
| `reader/render/crt_present.*` | What ties it to this emulator's present shaders. |

**These files are the same in every fork, and the Flycast Reader fork's are the ones to
change.** `tools/reader/crt.py sync` there writes `crt_shader.inc` and copies them to the
sibling forks beside it, each with its own `#include` paths and Kotlin package (and makes
the shader files the PPSSPP and Eden forks build theirs from); `crt.py check` says which
copy differs.

**A new slider** is a row in `kParams`, a place in `Pack` (the eight vectors are full: two
pairs already share a number each, as the shader's first lines say), its use in the shader,
`crt.py sync`, and a build. Neither settings page names a slider. The values are kept as
one line of text (`enabled=1;scanlines=0.550;...`): `picture/crt` in the desktop app's
`reader.ini`, the preference `reader_crt` on Android. A key is never renamed.

## In this emulator

The emulator puts the finished frame on the window with one of its present shaders (the "TV
shaders"). The CRT picture is one more, `PresentShader::READER_CRT`, entry `ps_reader_crt`:

- Each renderer reads its present shaders from one file and builds them all from it;
  `ReaderCrtPresent::Source` (`reader/render/crt_present.cpp`) is put after the file's own
  text, for OpenGL, Vulkan and Direct3D 11 and 12 (HLSL). No shader file holds a copy.
  Under Direct3D only the CRT entry is built from the lengthened text, and the plain copy
  stands in for it should it not build.
- Its 32 numbers travel in the present shaders' own constants: `DisplayConstantBuffer` grew
  from 96 to 128 bytes (`GSDevice.h`; so did the Vulkan and Direct3D 12 push constants and
  the shader files' blocks), and for this one shader `SetReaderCrt` writes the numbers in
  place of what the others read there. **The Metal renderer was not touched** and its
  check of that structure's size will fail: the fork is not built for macOS.
- `GSRenderer.cpp` picks the shader (`ReaderPresentShader`) and asks for the frame smoothed.

Whether a card's picture shows the CRT picture was not looked at.

## How it was checked

- **Tests** with made-up values: `crt_test.cpp`, part of `tools/reader/test-desktop.sh`.
- **In the app on Linux, Vulkan, drawn in software** (2026-10-13): a game's title screen
  with the picture switched on over the development channel, in a picture of the window:
  the bent glass, the corners, the phosphor, the lamp at the top.
- **OpenGL**: built, not run.
- **Direct3D 11 and 12**: the Windows package was built with it (2026-10-13). The CRT shader is built from a text of its own there, and without it the picture is the plain one, so a fault in it cannot take the other present shaders along. The HLSL was only put through glslang's HLSL reader,
  not Microsoft's compiler. **Never run.** These are what the Windows app draws with on many
  graphics cards: if the picture does not change there, the emulator's log says whether the
  shader was built, and *Settings, Graphics, Renderer* has Vulkan.
- **Not looked at**: the emulator's zoom, offsets and integer scaling with it, interlaced
  games, the sliders moved by hand.
