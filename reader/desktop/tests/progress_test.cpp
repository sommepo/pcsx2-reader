// SPDX-FileCopyrightText: PCSX2 Reader fork contributors
// SPDX-License-Identifier: GPL-3.0+

#include <cstring>
#include <string>
#include <catch2/catch_test_macros.hpp>
#include "reader/core/engine.h"

// Progress set by hand (reader.progress): a made-up game whose "routes" are 16-bit cells of
// made-up memory, as CLANNAD's profile has them.
namespace {
constexpr char Profile[] = R"lua(
-- reader: no hooks
local CELLS = 0x1000
reader.progress{
    title = "Routes",
    note = "Save the system data.",
    items = { { id = "a", label = "Route A", group = "Part 1" }, { id = "b", label = "Route B" } },
    done = function(id)
        local at = id == "a" and 0 or 2
        return mem.u16(CELLS + at) == 1
    end,
    mark = function(id, on)
        local at = id == "a" and 0 or 2
        mem.w16(CELLS + at, on and 1 or 0)
        if id == "b" and on then mem.w16(CELLS, 1) end -- B brings A with it
    end,
}
-- Written outside a mark: refused.
written_at_load = pcall(mem.w16, CELLS, 7)
)lua";
} // namespace

TEST_CASE("a profile's progress is read and marked through its memory", "[progress]") {
    std::string memory(0x2000, '\0');
    auto& engine = Reader::Engine::Instance();
    engine.StartDetached(
        [&memory](u64 address, std::size_t size) {
            return address < memory.size() ? memory.substr(address, size) : std::string{};
        },
        "MADE-00001",
        [&memory](u64 address, const std::string& bytes) {
            if (address + bytes.size() > memory.size()) {
                return false;
            }
            std::memcpy(memory.data() + address, bytes.data(), bytes.size());
            return true;
        });
    REQUIRE(engine.LoadProfile("made-up", Profile));
    std::string out;
    REQUIRE(engine.Execute("written_at_load", out));
    CHECK(out == "false");
    CHECK(memory[0x1000] == 0);

    auto list = engine.Progress();
    REQUIRE(list);
    CHECK(list->title == "Routes");
    CHECK(list->note == "Save the system data.");
    REQUIRE(list->items.size() == 2);
    CHECK(list->items[0].id == "a");
    CHECK(list->items[0].group == "Part 1");
    CHECK(list->items[1].label == "Route B");
    CHECK_FALSE(list->items[0].done);

    std::string error;
    REQUIRE(engine.MarkProgress("b", true, error));
    list = engine.Progress();
    CHECK(list->items[0].done);
    CHECK(list->items[1].done);
    CHECK(memory[0x1002] == 1);

    REQUIRE(engine.MarkProgress("a", false, error));
    CHECK_FALSE(engine.Progress()->items[0].done);
    // Still refused once marking is over.
    REQUIRE(engine.Execute("(pcall(mem.w16, 0x1000, 7))", out));
    CHECK(out == "false");
    CHECK(memory[0x1000] == 0);
    engine.StopDetached();
}

TEST_CASE("a profile that offers nothing has no progress", "[progress]") {
    std::string memory(0x100, '\0');
    auto& engine = Reader::Engine::Instance();
    engine.StartDetached([&memory](u64 address, std::size_t size) { return memory.substr(address, size); });
    REQUIRE(engine.LoadProfile("plain", "-- reader: no hooks\n"));
    CHECK_FALSE(engine.Progress());
    std::string error;
    CHECK_FALSE(engine.MarkProgress("a", true, error));
    CHECK_FALSE(error.empty());
    engine.StopDetached();
}
