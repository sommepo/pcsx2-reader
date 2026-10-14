// SPDX-FileCopyrightText: PCSX2 Reader fork contributors (from the Flycast Reader fork)
// SPDX-License-Identifier: GPL-2.0-or-later

// Save sync as the emulator uses it (host.h): a Dreamcast game's save is the folder named by
// the game's ID, which holds its memory card. Made-up saves; the tests that need a service
// are skipped without one (tools/reader/test-savesync.sh starts it).

#include <cstdlib>
#include <mutex>
#include <random>
#include <catch2/catch_test_macros.hpp>
#include "reader/save_sync/host.h"
#include "reader/save_sync/tests/test_util.h"

using namespace SaveSyncTest;
namespace Host = SaveSync::Host;

namespace {

std::string Env(const char* name) {
    const char* value = std::getenv(name);
    return value ? value : "";
}

std::string NewDisc() {
    static std::mt19937 random{std::random_device{}()};
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "SLZZ-%05u", static_cast<unsigned>(random() % 100000));
    return buffer;
}

/// An install of the app: its saves folder and save sync's own folder.
struct Install {
    explicit Install(std::string name_) : name{std::move(name_)} {}

    void Start(bool enabled = true) {
        Host::Start((scratch.path / "saves").string(), (scratch.path / "sync").string(),
                    [this](const std::string& text) {
                        std::scoped_lock lock{mutex};
                        said.push_back(text);
                    },
                    name);
        Host::Settings settings = Host::Get();
        settings.enabled = enabled;
        settings.addresses = Env("SAVESYNC_TEST_ADDRESS");
        settings.code = Env("SAVESYNC_TEST_CODE");
        Host::Set(settings);
    }
    void Save(const std::string& folder, const std::string& content) {
        Write(scratch.path / "saves" / folder / "save.bin", content);
        Host::NoteSaved();
    }
    [[nodiscard]] std::map<std::string, std::string> Saves() const {
        return Tree(scratch.path / "saves");
    }
    [[nodiscard]] std::vector<std::string> Said() {
        std::scoped_lock lock{mutex};
        return said;
    }

    std::string name;
    Scratch scratch;
    std::mutex mutex;
    std::vector<std::string> said;
};

#define NEEDS_SERVICE()                                                                           \
    if (Env("SAVESYNC_TEST_ADDRESS").empty()) {                                                   \
        SKIP("no service: run tools/reader/test-savesync.sh");                                    \
    }

} // namespace

TEST_CASE("A PlayStation 2 game's serial is a title of the service's kind, and back",
          "[savesync][ps2]") {
    for (const char* disc : {"SLPM-65550", "SLPS-25256", "SCPS-15001", "SLUS-20001", "PBPX-95503"}) {
        const std::string title = Host::TitleOf(disc);
        INFO(disc);
        REQUIRE(title.size() == 16);
        CHECK(Host::DiscOf(title) == disc);
    }
    CHECK(Host::TitleOf("SLPM-65550") == "534C504D00065550");
    CHECK(Host::TitleOf("SLPM-65550") != Host::TitleOf("SLPM-65551"));
    CHECK(Host::TitleOf("").empty());
    CHECK(Host::TitleOf("slpm-65550").empty());
    CHECK(Host::TitleOf("SLPM 65550").empty());
    CHECK(Host::TitleOf("SLPM_655.50").empty());
    CHECK(Host::TitleOf("SLPM-6555").empty());
    CHECK(Host::TitleOf("SLPM-655500").empty());
    CHECK(Host::TitleOf("SLP1-65550").empty());
    // Another console's titles are not this one's: the Switch's, the PSP's (whose serials
    // begin with U or N), the Dreamcast's.
    CHECK(Host::DiscOf("0100CB000A142000").empty());
    CHECK(Host::DiscOf("554C4A4D00005887").empty());
    CHECK(Host::DiscOf("4E504A4800050559").empty());
    CHECK(Host::TitleOf("ULJM-05887").empty());
    CHECK(Host::DiscOf("DCFFFFFFFFFFFFFF").empty());
    CHECK(Host::DiscOf("DC").empty());
}

TEST_CASE("A game's save is the folder named by its ID", "[savesync][ps2]") {
    CHECK(Host::IsSaveFolderOf("SLPM-65550", "SLPM-65550"));
    CHECK_FALSE(Host::IsSaveFolderOf("SLPM-65550", "SLPM-65551"));
    CHECK_FALSE(Host::IsSaveFolderOf("SLPM-65550", "SLPM-65550DATA"));
    // What save sync itself puts beside a folder while it replaces it.
    CHECK_FALSE(Host::IsSaveFolderOf("SLPM-65550", "SLPM-65550.savesync-new"));
    CHECK_FALSE(Host::IsSaveFolderOf("SLPM-65550", "SLPM-65550/../x"));
    CHECK_FALSE(Host::IsSaveFolderOf("My Homebrew", "My Homebrew"));

    Scratch scratch;
    Write(scratch.path / "SLPM-65550" / "save.bin", "a");
    Write(scratch.path / "SLPS-25256" / "save.bin", "b");
    Write(scratch.path / "notes.txt", "d");
    Write(scratch.path / "My Homebrew" / "save.bin", "e");
    std::filesystem::create_directories(scratch.path / "SCPS-15001");
    // A card the emulator made for a game that never saved has only its mark in it.
    Write(scratch.path / "SLUS-20001" / "_pcsx2_superblock", "");
    CHECK(Host::DiscsWithSaves(scratch.path.string()) ==
          std::vector<std::string>{"SLPM-65550", "SLPS-25256"});
}

TEST_CASE("Switched off, nothing is sent and nothing is said", "[savesync][ps2]") {
    Install thor{"Thor"};
    thor.Start(false);
    const std::string disc = NewDisc();
    thor.Save(disc, "chapter 1");
    Host::GameStarting(disc, "A made-up game");
    CHECK(Host::RunningDisc() == disc);
    thor.Save(disc, "chapter 2");
    Host::GameStopped();
    CHECK(Host::RunningDisc().empty());
    CHECK(Host::Wait(5000));
    CHECK(thor.Said().empty());
    CHECK(Host::Conflicts().empty());
    Host::Shutdown();
    CHECK_FALSE(Host::Started());
}

TEST_CASE("The settings are kept from one start to the next", "[savesync][ps2]") {
    Install thor{"Thor"};
    Host::Start((thor.scratch.path / "saves").string(), (thor.scratch.path / "sync").string(),
                {}, "Thor");
    CHECK(Host::Get().device == "Thor");
    CHECK_FALSE(Host::Get().enabled);
    Host::Settings settings;
    settings.enabled = true;
    settings.addresses = "127.0.0.1:1";
    settings.code = "AAAA-BBBB-CCCC";
    settings.device = "My Thor";
    Host::Set(settings);
    Host::Shutdown();
    Host::Start((thor.scratch.path / "saves").string(), (thor.scratch.path / "sync").string(),
                {}, "Another name");
    const Host::Settings kept = Host::Get();
    CHECK(kept.enabled);
    CHECK(kept.addresses == "127.0.0.1:1");
    CHECK(kept.code == "AAAA-BBBB-CCCC");
    CHECK(kept.device == "My Thor");
    Host::Shutdown();
}

TEST_CASE("A game's memory card goes to another device and back", "[savesync][ps2]") {
    NEEDS_SERVICE();
    const std::string disc = NewDisc();
    Install thor{"Thor"};
    Install z13{"Z13"};

    thor.Start();
    thor.Save(disc, "chapter 1");
    thor.Save("SLUS-99999", "another game's");
    Host::GameStarting(disc, "A made-up game");
    REQUIRE(Host::Wait(10000));
    // While it runs: the game saves again.
    thor.Save(disc, "chapter 2");
    REQUIRE(Host::Wait(10000));
    Host::GameStopped();
    Host::Shutdown();

    z13.Start();
    Host::GameStarting(disc, "A made-up game");
    CHECK(z13.Saves() ==
          std::map<std::string, std::string>{{disc + "/save.bin", "chapter 2"}});
    z13.Save(disc, "chapter 3");
    REQUIRE(Host::Wait(10000));
    Host::GameStopped();
    Host::Shutdown();

    thor.Start();
    Host::GameStarting(disc, "A made-up game");
    CHECK(thor.Saves() == std::map<std::string, std::string>{
                              {disc + "/save.bin", "chapter 3"},
                              {"SLUS-99999/save.bin", "another game's"}});
    // What was replaced here is kept.
    const auto backups = Tree(thor.scratch.path / "sync" / "backups" / Host::TitleOf(disc));
    CHECK_FALSE(backups.empty());
    Host::GameStopped();
    Host::Shutdown();
}

TEST_CASE("Two devices both played: the user chooses, in the menu", "[savesync][ps2]") {
    NEEDS_SERVICE();
    const std::string disc = NewDisc();
    const std::string title = Host::TitleOf(disc);
    Install thor{"Thor"};
    Install z13{"Z13"};

    thor.Start();
    thor.Save(disc, "chapter 1");
    Host::GameStarting(disc, "A made-up game");
    Host::GameStopped();
    REQUIRE(Host::Wait(10000));
    Host::Shutdown();

    z13.Start();
    Host::GameStarting(disc, "A made-up game");
    REQUIRE(z13.Saves().size() == 1);
    Host::GameStopped();
    REQUIRE(Host::Wait(10000));
    // The Z13 plays on without the service (the app is not even running here: started with
    // sync on, it would look at every save at once)...
    Host::Shutdown();
    z13.Save(disc, "the Z13's chapter 2");

    // ...and so does the Thor, which reaches it first.
    thor.Start();
    Host::GameStarting(disc, "A made-up game");
    thor.Save(disc, "the Thor's chapter 2");
    Host::GameStopped();
    REQUIRE(Host::Wait(10000));
    Host::Shutdown();

    // Back at the service, the Z13 is told and keeps its own save for now.
    z13.Start();
    Host::GameStarting(disc, "A made-up game");
    REQUIRE(Host::Conflicts().size() == 1);
    CHECK(Host::Conflicts()[0].title == title);
    CHECK(Host::Conflicts()[0].disc == disc);
    CHECK(Host::Conflicts()[0].name == "A made-up game");
    CHECK(Host::Conflicts()[0].message.find("Thor: saved ") != std::string::npos);
    // Said now that the game starts (and before, when the app's start found it).
    REQUIRE_FALSE(z13.Said().empty());
    CHECK(z13.Said().back().find("A made-up game has two different saves") !=
          std::string::npos);
    CHECK(z13.Saves().at(disc + "/save.bin") == "the Z13's chapter 2");
    // Nothing of it goes up meanwhile.
    z13.Save(disc, "the Z13's chapter 3");
    REQUIRE(Host::Wait(10000));

    // The other save cannot take this one's place under a running game.
    Host::Resolve(title, false);
    REQUIRE(Host::Wait(10000));
    CHECK(Host::Status().find("Stop the game first") == 0);
    CHECK(z13.Saves().at(disc + "/save.bin") == "the Z13's chapter 3");
    Host::GameStopped();
    REQUIRE(Host::Wait(10000));
    CHECK(Host::Conflicts().size() == 1);

    Host::Resolve(title, false);
    REQUIRE(Host::Wait(10000));
    CHECK(Host::Conflicts().empty());
    CHECK(z13.Saves().at(disc + "/save.bin") == "the Thor's chapter 2");
    const auto backups = Tree(z13.scratch.path / "sync" / "backups" / title);
    REQUIRE(backups.size() == 1);
    CHECK(backups.begin()->second == "the Z13's chapter 3");
    Host::Shutdown();

    // The Thor still has its own, and nothing to choose.
    thor.Start();
    Host::GameStarting(disc, "A made-up game");
    CHECK(Host::Conflicts().empty());
    CHECK(thor.Saves().at(disc + "/save.bin") == "the Thor's chapter 2");
    Host::GameStopped();
    Host::Shutdown();
}

TEST_CASE("A conflict found when the app starts is listed, and this device's save can be kept",
          "[savesync][ps2]") {
    NEEDS_SERVICE();
    const std::string disc = NewDisc();
    const std::string title = Host::TitleOf(disc);
    Install thor{"Thor"};
    Install z13{"Z13"};

    thor.Start();
    thor.Save(disc, "chapter 1");
    Host::GameStarting(disc, "A made-up game");
    Host::GameStopped();
    REQUIRE(Host::Wait(10000));
    Host::Shutdown();
    z13.Start();
    Host::GameStarting(disc, "");
    Host::GameStopped();
    REQUIRE(Host::Wait(10000));
    Host::Shutdown();
    z13.Save(disc, "the Z13's chapter 2");
    thor.Start();
    Host::GameStarting(disc, "A made-up game");
    thor.Save(disc, "the Thor's chapter 2");
    Host::GameStopped();
    REQUIRE(Host::Wait(10000));
    Host::Shutdown();

    // The app's start looks at every game with a save here.
    z13.Start();
    REQUIRE(Host::Wait(10000));
    REQUIRE(Host::Conflicts().size() == 1);
    CHECK(Host::Conflicts()[0].disc == disc);
    Host::Resolve(title, true);
    REQUIRE(Host::Wait(10000));
    CHECK(Host::Conflicts().empty());
    CHECK(z13.Saves().at(disc + "/save.bin") == "the Z13's chapter 2");
    Host::Shutdown();

    thor.Start();
    Host::GameStarting(disc, "A made-up game");
    CHECK(thor.Saves().at(disc + "/save.bin") == "the Z13's chapter 2");
    Host::GameStopped();
    Host::Shutdown();
}

TEST_CASE("What the background did names its game", "[savesync][ps2]") {
    NEEDS_SERVICE();
    const std::string disc = NewDisc();
    Install thor{"Thor"};
    thor.Save(disc, "chapter 1");
    thor.Start();
    Host::SyncAll();
    REQUIRE(Host::Wait(10000));
    REQUIRE_FALSE(thor.Said().empty());
    CHECK(thor.Said().back() == "Save sync: this device's save was sent. (" + disc + ")");
    CHECK(Host::Status() == thor.Said().back());

    // Once the game has been started here its name is known; and what is said at its own
    // start needs no name.
    Host::GameStarting(disc, "A made-up game");
    Host::GameStopped();
    REQUIRE(Host::Wait(10000));
    Host::Shutdown();
    thor.Save(disc, "chapter 2");
    thor.Start();
    REQUIRE(Host::Wait(10000));
    CHECK(thor.Said().back() == "Save sync: this device's save was sent. (A made-up game)");
    Host::Shutdown();
}
