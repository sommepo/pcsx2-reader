// SPDX-FileCopyrightText: PCSX2 Reader fork contributors
// SPDX-License-Identifier: GPL-3.0+

#include <cstring>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <QDir>
#include <QFile>
#include "reader/core/engine.h"
#include "reader/desktop/catalog.h"

// Touch play (READER.md, "Playing by touch"): a profile says which answer of a choice the game's
// cursor is on (reader.cursor), which a tap on an answer is turned into presses of the d-pad
// with. First the engine's side, then each visual novel's profile against made-up memory laid
// out as the game's (no game data: the texts are made up, the places are the profile's own).

using Catch::Matchers::ContainsSubstring;

namespace {

/// The engine, detached, with made-up main memory and a program of the given build.
struct MadeUpGame {
    std::string memory = std::string(32 * 1024 * 1024, '\0');
    Reader::Engine& engine = Reader::Engine::Instance();

    explicit MadeUpGame(const std::string& build = {}, const std::string& game_id = "MADE-00001") {
        engine.StartDetached(
            [this](u64 address, std::size_t size) {
                return address < memory.size() ? memory.substr(address, size) : std::string{};
            },
            game_id);
        if (!build.empty()) {
            engine.OnModuleLoaded("main", 0x100000, 0x400000, build);
        }
    }
    ~MadeUpGame() {
        engine.StopDetached();
    }

    bool Load(const std::string& source) {
        return engine.LoadProfile("made-up", source);
    }
    /// One of the bundled profiles, as it is in the repository.
    bool LoadBundled(const std::string& serial) {
        std::ifstream file(std::string(READER_BUNDLED_PROFILE_DIR) + "/" + serial + ".lua",
                           std::ios::binary);
        std::stringstream source;
        source << file.rdbuf();
        const bool loaded = engine.LoadProfile(serial, source.str());
        if (!loaded) {
            UNSCOPED_INFO(engine.LastError());
        }
        return loaded;
    }
    std::string Lua(const std::string& code) {
        std::string out;
        engine.Execute(code, out);
        return out;
    }
    void Frames(int count = 1) {
        for (int i = 0; i < count; ++i) {
            engine.OnFrame();
        }
    }
    void Put(u32 address, const std::string& bytes) {
        std::memcpy(memory.data() + address, bytes.data(), bytes.size());
    }
    template <typename T>
    void Set(u32 address, T value) {
        std::memcpy(memory.data() + address, &value, sizeof(value));
    }
    /// The cursor the profile reports: the answer, "-1" for none yet, "none" without a choice;
    /// with " sideways" when the answers stand side by side.
    std::string Cursor() const {
        const auto cursor = engine.Cursor();
        if (!cursor) {
            return "none";
        }
        return std::to_string(cursor->index) + (cursor->horizontal ? " sideways" : "");
    }
};

/// Shift-JIS of a few kana and signs, for made-up answers (the profiles decode with text.sjis).
std::string Sjis(const std::string& name) {
    if (name == "hai") {
        return "\x82\xcd\x82\xa2"; // はい
    }
    if (name == "iie") {
        return "\x82\xa2\x82\xa2\x82\xa6"; // いいえ
    }
    return "\x82\xa0\x81\x42"; // あ。
}

} // namespace

TEST_CASE("A profile says which answer of a choice the cursor is on", "[touch]") {
    MadeUpGame game;
    REQUIRE(game.Load("-- reader: no hooks\n"));
    CHECK(game.Cursor() == "none");
    CHECK(game.Lua("reader.cursor(2)").empty());
    CHECK(game.Cursor() == "2");
    game.Lua("reader.cursor(-1, 'horizontal')");
    CHECK(game.Cursor() == "-1 sideways");
    game.Lua("reader.cursor(nil)");
    CHECK(game.Cursor() == "none");
    CHECK_THAT(game.Lua("reader.cursor(-2)"), ContainsSubstring("an answer"));
    CHECK_THAT(game.Lua("reader.cursor(1, 'diagonal')"), ContainsSubstring("horizontal"));
    game.Lua("reader.cursor(0)");
    // A profile loaded anew, or none, says nothing of the last one's cursor.
    REQUIRE(game.Load("-- reader: no hooks\n"));
    CHECK(game.Cursor() == "none");
    game.Lua("reader.cursor(1)");
    game.engine.UnloadProfile();
    CHECK(game.Cursor() == "none");
}

TEST_CASE("Never7: the cursor is on no answer until the d-pad is pressed, then on the one it says",
          "[touch][profiles]") {
    MadeUpGame game{"9D260CAF", "SLPS-25256"};
    REQUIRE(game.LoadBundled("SLPS-25256"));
    constexpr u32 answer_text = 0x5cbe57, answer_size = 66, answer_count = 0x398630;
    constexpr u32 choice_box = 0x397f4c, cursor = 0x53d896;
    game.Frames(2);
    CHECK(game.Cursor() == "none");
    // A choice of two comes up, nothing lit yet (0 kept from no earlier choice).
    game.Put(answer_text, Sjis("hai"));
    game.Put(answer_text + answer_size, Sjis("iie"));
    game.Set<u32>(answer_count, 2);
    game.Set<u32>(choice_box, 240);
    game.Frames();
    CHECK(game.Cursor() == "-1");
    game.Set<u16>(cursor, 1); // the first press of the d-pad: the first answer
    game.Frames();
    CHECK(game.Cursor() == "0");
    game.Set<u16>(cursor, 2);
    game.Frames();
    CHECK(game.Cursor() == "1");
    game.Set<u16>(cursor, 7); // not an answer of this choice
    game.Frames();
    CHECK(game.Cursor() == "-1");
    // Picked: the box goes, the number stays in memory.
    game.Set<u16>(cursor, 2);
    game.Set<u32>(choice_box, 0);
    game.Frames();
    CHECK(game.Cursor() == "none");
    // The next choice begins on no answer whatever the last one left (the game sets 0).
    game.Set<u16>(cursor, 0);
    game.Put(answer_text + 2 * answer_size, Sjis("a"));
    game.Set<u32>(answer_count, 3);
    game.Set<u32>(choice_box, 96);
    game.Frames();
    CHECK(game.Cursor() == "-1");
}

namespace {
/// Ever17 and Remember11: one engine, the same rules at each program's places.
void KidChoiceCursor(const std::string& serial, const std::string& build, u32 answers, u32 cursor) {
    MadeUpGame game{build, serial};
    REQUIRE(game.LoadBundled(serial));
    const u32 answer_text = answers + 8, answer_size = 64;
    game.Frames(2);
    CHECK(game.Cursor() == "none");
    game.Set<u8>(cursor, 0xff); // on no answer until the d-pad is first pressed
    game.Put(answer_text, Sjis("hai"));
    game.Put(answer_text + answer_size, Sjis("iie"));
    game.Put(answer_text + 2 * answer_size, Sjis("a"));
    game.Set<u32>(answers, 3);
    game.Frames();
    CHECK(game.Cursor() == "-1");
    game.Set<u8>(cursor, 0);
    game.Frames();
    CHECK(game.Cursor() == "0");
    game.Set<u8>(cursor, 2);
    game.Frames();
    CHECK(game.Cursor() == "2");
    game.Set<u8>(cursor, 3); // past the last answer: not one of them
    game.Frames();
    CHECK(game.Cursor() == "-1");
    game.Set<u8>(cursor, 1);
    game.Set<u32>(answers, 0); // picked: the choice is gone, the number stays
    game.Frames();
    CHECK(game.Cursor() == "none");
}
} // namespace

TEST_CASE("Ever17: the cursor is a byte beside the choice, 0xff until the d-pad is pressed",
          "[touch][profiles]") {
    KidChoiceCursor("SLPM-65421", "C577D399", 0xc2b648, 0xc2b572);
}

TEST_CASE("Remember11: the cursor is a byte beside the choice, 0xff until the d-pad is pressed",
          "[touch][profiles]") {
    KidChoiceCursor("SLPM-65550", "B33522CD", 0xb17ea0, 0xb17dca);
}

TEST_CASE("CLANNAD: the cursor is reported while a choice waits, 0xffff being none",
          "[touch][profiles]") {
    MadeUpGame game{"7800DC84", "SLPM-66302"};
    REQUIRE(game.LoadBundled("SLPM-66302"));
    constexpr u32 state = 0x3775b8, count = 0x3775b0, answers = 0x377290, cursor = 0x60cdda;
    game.Frames(2);
    CHECK(game.Cursor() == "none");
    game.Put(answers, Sjis("hai"));
    game.Put(answers + 100, Sjis("iie"));
    game.Set<u16>(count, 2);
    game.Set<u16>(cursor, 0xffff);
    game.Frames();
    CHECK(game.Cursor() == "none"); // the answers are there, but no choice waits yet
    game.Set<u32>(state, 0x10);
    game.Frames();
    CHECK(game.Cursor() == "-1");
    game.Set<u16>(cursor, 0);
    game.Frames();
    CHECK(game.Cursor() == "0");
    game.Set<u16>(cursor, 1);
    game.Frames();
    CHECK(game.Cursor() == "1");
    game.Set<u32>(state, 0x501); // picked: the script goes on, the number stays
    game.Frames();
    CHECK(game.Cursor() == "none");
}

TEST_CASE("The visual novels' scripts say they are played by presses, and the other games' nothing",
          "[touch][profiles]") {
    const QStringList novels{
        QStringLiteral("SLPS-25256"), // Never7
        QStringLiteral("SLPM-65421"), // Ever17
        QStringLiteral("SLPM-65550"), // Remember11
        QStringLiteral("SLPM-66302"), // CLANNAD
        QStringLiteral("SLPS-25918"), // アマガミ
        QStringLiteral("SLPM-66913"), // ひぐらしのなく頃に祭
        QStringLiteral("SLPM-65320"), // EVE burst error PLUS
    };
    const QDir folder(QStringLiteral(READER_BUNDLED_PROFILE_DIR));
    int seen = 0;
    for (const QString& name : folder.entryList({QStringLiteral("*.lua")}, QDir::Files)) {
        QFile file(folder.filePath(name));
        REQUIRE(file.open(QIODevice::ReadOnly));
        const QString id = name.left(name.size() - 4);
        const auto info = ReaderApp::Catalog::Describe(id, QString::fromUtf8(file.readAll()), false);
        INFO(id.toStdString());
        CHECK(info.touch == (novels.contains(id) ? QStringLiteral("press") : QString()));
        seen += novels.contains(id) ? 1 : 0;
    }
    CHECK(seen == novels.size());
}

namespace {
/// アマガミ's memory as its script reads it (SLPS-25918.lua's comments): the scene's text values,
/// the draw list's pool and its items. Characters are Shift-JIS codes.
struct MadeUpAmagami : MadeUpGame {
    static constexpr u32 Scripts = 0x3a6bd8, Pool = 0x3dc000, Things = 0x554000;
    static constexpr u32 DrawnList = 0x2847c8, TextKind = 0x300, TextType = 0x010a994c;
    u32 next_entry = Pool, next_thing = Things;
    std::vector<std::pair<u32, u32>> placed; ///< pool entries made, to clear

    MadeUpAmagami() : MadeUpGame("D72C9E2A", "SLPS-25918") {}

    /// A text value at `at` with its characters at `chars` (16 bits each, a code's bytes
    /// swapped, as the game keeps them).
    void Text(u32 at, u32 chars, const std::vector<u16>& codes) {
        Set<u32>(at, static_cast<u32>(codes.size()));
        Set<u32>(at + 4, chars);
        Set<u32>(at + 8, 0);
        Set<u32>(at + 12, TextKind);
        Set<u32>(at + 16, TextType);
        for (std::size_t i = 0; i < codes.size(); ++i) {
            Set<u16>(chars + static_cast<u32>(i) * 2, codes[i]);
        }
    }
    void Entry(u32 thing, u32 size) {
        Set<u32>(next_entry, DrawnList);
        Set<u32>(next_entry + 12, thing);
        Set<u32>(next_entry + 16, size);
        next_entry += 0x20;
    }
    /// A character's picture (24 by 14 in a field) at x, y of the 640 by 448 picture.
    void Glyph(int x, int y, int sheet) {
        const u32 thing = next_thing;
        next_thing += 0x60;
        Set<u16>(thing + 0x40, static_cast<u16>(sheet * 16));
        Set<u16>(thing + 0x42, 0);
        Set<u16>(thing + 0x48, static_cast<u16>((x + 1728) * 16));
        Set<u16>(thing + 0x4a, static_cast<u16>((y / 2 + 1936) * 16));
        Set<u16>(thing + 0x58, static_cast<u16>((x + 24 + 1728) * 16));
        Set<u16>(thing + 0x5a, static_cast<u16>((y / 2 + 14 + 1936) * 16));
        Entry(thing, 0x60);
    }
    /// The bar that lights an answer: 560 by 36, its top at `top`.
    u32 Bar(int top) {
        const u32 thing = next_thing;
        next_thing += 0x50;
        Set<u16>(thing + 0x40, static_cast<u16>((40 + 1728) * 16));
        Set<u16>(thing + 0x42, static_cast<u16>((top / 2 + 1936) * 16));
        Set<u16>(thing + 0x48, static_cast<u16>((600 + 1728) * 16));
        Set<u16>(thing + 0x4a, static_cast<u16>((top / 2 + 18 + 1936) * 16));
        Entry(thing, 0x50);
        return thing;
    }
};
} // namespace

TEST_CASE("アマガミ: the answer lit is the one under the bar the game draws", "[touch][profiles]") {
    MadeUpAmagami game;
    REQUIRE(game.LoadBundled("SLPS-25918"));
    // A choice of three answers, each a row that begins with its mark ・ (made-up text: あい,
    // いう, うえ). The answers' values lie one after the other downwards from the first, which
    // the scene's script points at.
    constexpr u16 mark = 0x8145, a = 0x82a0, i = 0x82a2, u = 0x82a4, e = 0x82a6;
    const std::vector<std::vector<u16>> answers{{mark, a, i}, {mark, i, u}, {mark, u, e}};
    constexpr u32 first = 0x700000;
    for (std::size_t k = 0; k < answers.size(); ++k) {
        game.Text(first - static_cast<u32>(k) * 40, 0x710000 + static_cast<u32>(k) * 0x100, answers[k]);
    }
    game.Set<u32>(MadeUpAmagami::Scripts, first + 12);
    // Their pictures: rows 44 apart from 296, characters 30 apart, one sheet place a character.
    const std::map<u16, int> sheet{{mark, 1}, {a, 2}, {i, 3}, {u, 4}, {e, 5}};
    for (std::size_t row = 0; row < answers.size(); ++row) {
        for (std::size_t k = 0; k < answers[row].size(); ++k) {
            game.Glyph(60 + static_cast<int>(k) * 30, 296 + static_cast<int>(row) * 44, sheet.at(answers[row][k]));
        }
    }
    game.Frames(80);
    REQUIRE(game.engine.ChoiceShown());
    CHECK(game.Cursor() == "-1"); // no bar until the d-pad is pressed
    const u32 bar = game.Bar(290);
    game.Frames(8);
    CHECK(game.Cursor() == "0");
    game.Set<u16>(bar + 0x42, static_cast<u16>((378 / 2 + 1936) * 16)); // moved to the third
    game.Set<u16>(bar + 0x4a, static_cast<u16>((378 / 2 + 18 + 1936) * 16));
    game.Frames(8);
    CHECK(game.Cursor() == "2");
    // A bar of another size (a box, the SELECT label) is not the cursor.
    game.Set<u16>(bar + 0x48, static_cast<u16>((172 + 1728) * 16));
    game.Frames(8);
    CHECK(game.Cursor() == "-1");
}
