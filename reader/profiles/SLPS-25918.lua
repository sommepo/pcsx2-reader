-- reader: no hooks
-- reader: title アマガミ
-- reader: reads The message window's text with furigana, each message when it begins, and choices
-- reader: version SLPS-25918
-- アマガミ (Enterbrain, PS2, 2009), SLPS-25918. Program D72C9E2A.
--
-- Two things are read. **What is said** comes from the scene's script: the game keeps a row
-- of values for it (20 bytes each: two numbers, nothing, a kind, something of the kind's),
-- a message's text among them, and for each script that runs a number that says which value
-- it used last; while a message is up, the scene's stays 12 bytes past the message's text.
-- **Where the characters stand** comes from what the game draws: its list of things to
-- draw has a picture for every character on the screen (24 by 14 in a field of the
-- interlaced picture, twice each for the shadow), with its place. Nothing in a picture says
-- which character it is, so the pictures are counted off against the text: the last of them,
-- in reading order, are the message's (a name in front of a message has pictures too, and
-- the message before may still stand in the rows above).
-- The number does not stay at a message while the message is up (for a line with a voice it
-- is hardly ever there), so the message is made out from the pictures: when they have
-- stopped coming, every text near where the scripts are is tried against them, and the one
-- that fits is the message: as many characters, rows breaking where its rows break, the
-- same picture wherever a character is the same. Which picture is which character is
-- learnt from every message made out, and a text is taken at once when half its pictures
-- are known to be its characters. Before that (after a start) a message is taken when it
-- has stood for two seconds and is the only text that fits, or when a script was seen at
-- it; short messages may go unreported until then. A message is reported when it has come
-- up in full.
--
-- A text value is its length (in Shift-JIS bytes) and where its characters are: 16-bit numbers, a
-- Shift-JIS character with its two bytes the other way round, or one byte for the marks
-- inside a message: ／ begins a new row, W and a number waits, V and a number is the voice,
-- R word R number reading puts a reading (of so many characters) over the word. A choice is the answers one after the
-- other, each beginning with ・, with the script at the first.

local BUILD = "D72C9E2A"
if reader.build_id ~= BUILD then
    error("this profile is for Amagami SLPS-25918 (program " .. BUILD ..
          "), the game is " .. tostring(reader.build_id))
end

local SCRIPTS, SCRIPTS_COUNT = 0x003a6bd8, 12   -- where the running scripts are, one number each
                                                -- (the value a script is at is 12 bytes before)
local AT, VALUE_SIZE = -12, 20
local V_LENGTH, V_CHARS, V_KIND, TEXT_KIND, TEXT_TYPE = 0x00, 0x04, 0x0c, 0x300, 0x010a994c
local NEAR = 0x3000    -- a scene's texts lie within so many bytes of where its script is
local LENGTH_MAX = 800
-- The list of things to draw: a pool of 32-byte entries (the list it is in, the next entry,
-- something, the thing, its size); a thing with a picture is 0x60 bytes: at 0x48 and 0x58
-- the two corners (x and y as 16-bit numbers in sixteenths), in the console's own
-- coordinates, whose origin for this picture is (1728, 1936) with every second line.
local POOL, POOL_END, ENTRY_SIZE = 0x003dc000, 0x003e9000, 0x20
local DRAWN_LIST, THING_SIZE = 0x002847c8, 0x60
local THINGS, THINGS_END = 0x00554000, 0x00578000
local ORIGIN_X, ORIGIN_Y = 1728, 1936
local GLYPH_W, GLYPH_H = 24, 14               -- a character's picture, in a field
local WINDOW_TOP, WINDOW_BOTTOM = 270, 430    -- the message window, in the picture of 448 lines
local CELL_HEIGHT = 28
local EVERY, STEADY, STOOD, ALONE_MIN = 4, 3, 20, 8    -- the pictures are looked at every so many frames, and have to
                               -- be the same so many times running
local ANSWER_MARK, SPACE = "・", "　"
local NAME_GLYPHS = 3
local RECENT, LATELY = 3600, 240    -- frames a text is kept after a script was last at it
local SHEET = 1024     -- the sheet of characters is so wide and high; a place in it comes with any multiple added
local ANSWERS_MAX = 8

local function valid(p)
    return p and p >= 0x00100000 and p < 0x02000000
end

-- The text value at `address` as a list of units: a character (UTF-8) or one byte below 0x80.
local function read_text(address)
    local head = mem.read(address, VALUE_SIZE)
    if not head or #head < VALUE_SIZE then return nil end
    local length, chars = string.unpack("<I4I4", head, V_LENGTH + 1)
    local kind = string.unpack("<I4", head, V_KIND + 1)
    if kind ~= TEXT_KIND or not valid(chars) or length < 2 or length > LENGTH_MAX then return nil end
    -- The length is the text's in Shift-JIS: at most twice as many bytes here, a zero ends it.
    local raw = mem.read(chars, length * 2)
    if not raw or #raw < length * 2 then return nil end
    local units = {}
    for at = 1, #raw - 1, 2 do
        local low, high = raw:byte(at), raw:byte(at + 1)
        if low == 0 and high == 0 then
            raw = raw:sub(1, at - 1)
            break
        end
        if high == 0 then
            units[#units + 1] = string.char(low)
        else
            units[#units + 1] = text.sjis(string.char(high, low))
        end
    end
    return units, raw
end

local function is_digit(u) return u and #u == 1 and u >= "0" and u <= "9" end

-- A message's units as rows of characters, with the readings: { rows = { { ch... } },
-- ruby = { { row, first, count, reading } } }.
local function parse(units)
    local rows, row, ruby = {}, {}, {}
    local at = 1
    local function number()
        local n = 0
        while is_digit(units[at]) do
            n = n * 10 + tonumber(units[at])
            at = at + 1
        end
        return n
    end
    while at <= #units do
        local u = units[at]
        if u == "／" then
            rows[#rows + 1] = row
            row = {}
            at = at + 1
        elseif u == "R" then
            -- R word R count reading
            at = at + 1
            local first = #row + 1
            while at <= #units and units[at] ~= "R" do
                row[#row + 1] = units[at]
                at = at + 1
            end
            at = at + 1
            local count, reading = number(), {}
            for _ = 1, count do
                reading[#reading + 1] = units[at] or ""
                at = at + 1
            end
            if #row >= first then
                ruby[#ruby + 1] = { #rows + 1, first, #row - first + 1, table.concat(reading) }
            end
        elseif #u == 1 and u >= "A" and u <= "Z" then
            -- W and a number waits, V and a number is the voice, others move a face: a mark
            -- is a capital letter and the letters and digits after it.
            at = at + 1
            while units[at] and #units[at] == 1 and units[at]:match("%w") do at = at + 1 end
        elseif #u == 1 and u:byte() < 0x20 then
            at = at + 1
        else
            row[#row + 1] = u
            at = at + 1
        end
    end
    rows[#rows + 1] = row
    return { rows = rows, ruby = ruby }
end

-- Whether units are a text to read: kana or kanji in it, and nothing that is not a character.
local function japanese(units)
    local found = false
    for _, u in ipairs(units) do
        local b = u:byte(1)
        if #u == 1 then
            if b < 0x20 or b > 0x7e then return false end
        elseif #u == 3 then
            if b >= 0xe3 and b <= 0xe9 then found = true end
            if u == "\239\191\189" or (b == 0xef and u:byte(2) >= 0xbd and u ~= "～") then
                -- not a character, or half-width kana, which a message has none of
                if u:byte(2) == 0xbd and u:byte(3) >= 0xa1 or u:byte(2) == 0xbe then return false end
                if u == "\239\191\189" then return false end
            end
        else
            return false
        end
    end
    return found
end

-- A message's text: its rows are one text, the space a later row is set in by left out.
-- `slots` says of each character of the text whether the game draws a picture for it (it
-- draws none for a space).
local function compose_message(parsed)
    local b, ruby, slots = reader.builder(), {}, {}
    local starts = {}
    for index, row in ipairs(parsed.rows) do
        starts[index] = {}
        local leading = index > 1
        for k, ch in ipairs(row) do
            starts[index][k] = b:length()
            if leading and ch == SPACE then
                -- left out
            else
                leading = false
                b:add(ch)
                slots[#slots + 1] = ch ~= SPACE and "drawn" or "space"
            end
        end
        starts[index][#row + 1] = b:length()
    end
    for _, group in ipairs(parsed.ruby) do
        local from, to = starts[group[1]][group[2]], starts[group[1]][group[2] + group[3]]
        if from and to and to > from then ruby[#ruby + 1] = { from, to - from, group[4] } end
    end
    return b:text(), ruby, slots
end

-- A choice's text: the answers a line each, without their marks.
local function compose_choice(answers)
    local b, slots = reader.builder(), {}
    for index, row in ipairs(answers) do
        if index > 1 then
            b:add("\n")
            slots[#slots + 1] = "break"
        end
        slots[#slots + 1] = "mark"
        for k = 2, #row do
            b:add(row[k])
            slots[#slots + 1] = row[k] ~= SPACE and "drawn" or "space"
        end
    end
    return b:text(), slots
end

-- The characters' pictures in the message window, in reading order: { x, y, id } in the
-- picture of 640 by 448; `id` is where the picture is in the game's sheet of characters, the
-- same for the same character. A character is drawn twice, the second a little to the right
-- and below.
local function drawn_glyphs()
    local pool = mem.read(POOL, POOL_END - POOL)
    local things = mem.read(THINGS, THINGS_END - THINGS)
    if not pool or not things then return nil end
    local found, seen = {}, {}
    for at = 1, #pool - ENTRY_SIZE + 1, ENTRY_SIZE do
        local list, _, _, thing, size = string.unpack("<I4I4I4I4I4", pool, at)
        if list == DRAWN_LIST and size == THING_SIZE and thing >= THINGS and thing + THING_SIZE <= THINGS_END then
            local o = thing - THINGS
            local u, v = string.unpack("<I2I2", things, o + 0x40 + 1)
            local x0, y0 = string.unpack("<I2I2", things, o + 0x48 + 1)
            local x1, y1 = string.unpack("<I2I2", things, o + 0x58 + 1)
            if x1 - x0 == GLYPH_W * 16 and y1 - y0 == GLYPH_H * 16 then
                local x, y = x0 // 16 - ORIGIN_X, (y0 // 16 - ORIGIN_Y) * 2
                if y >= WINDOW_TOP and y <= WINDOW_BOTTOM and x >= 0 and x < 640 and not seen[x * 1024 + y] then
                    seen[x * 1024 + y] = true
                    found[#found + 1] = { x, y, ((u // 16) % SHEET) * 65536 + (v // 16) % SHEET }
                end
            end
        end
    end
    -- Without the shadows.
    local glyphs = {}
    for _, g in ipairs(found) do
        if not seen[(g[1] - 2) * 1024 + g[2] - 2] then glyphs[#glyphs + 1] = g end
    end
    table.sort(glyphs, function(a, b)
        if a[2] ~= b[2] then return a[2] < b[2] end
        return a[1] < b[1]
    end)
    return glyphs
end

-- What picture is which character, learnt from the messages that were made out.
local learnt = {}
local drawn_as = {}   -- and which character is which picture
local blank          -- the picture of nothing, which a name is set out with

-- Whether the pictures from `start` on can be the first characters of `drawn` (a list of
-- characters, "\n" between rows): the rows breaking where they break, and the same picture
-- wherever the character is the same and only there. Also whether they are all of it, how
-- many pictures there are, how many of them are known to be their characters and how many
-- are known as others.
local function fits(drawn, glyphs, start)
    local at = start - 1
    local by_char, by_id = {}, {}
    local first = true
    local agree, differ = 0, 0
    local complete = true
    for _, ch in ipairs(drawn) do
        if ch == "\n" then
            first = true
        else
            at = at + 1
            local g = glyphs[at]
            if not g then
                complete = false
                break
            end
            -- A row begins where the y changes, and nowhere else inside the text.
            if at > start and (glyphs[at - 1][2] ~= g[2]) ~= first then return false end
            first = false
            if by_char[ch] and by_char[ch] ~= g[3] then return false end
            if by_id[g[3]] and by_id[g[3]] ~= ch then return false end
            by_char[ch], by_id[g[3]] = g[3], ch
            local known = learnt[g[3]]
            if known == ch then
                agree = agree + 1
            elseif known or (drawn_as[ch] and drawn_as[ch] ~= g[3]) then
                differ = differ + 1
            end
        end
    end
    if at < #glyphs then return false end
    return true, complete, #glyphs - start + 1, agree, differ
end

-- The texts of the scene: every text value near where the scripts are. { address = { units, raw } }
local TEXT_VALUE = string.pack("<I4I4I4", 0, TEXT_KIND, TEXT_TYPE)
local function scene_texts(slots)
    local out = {}
    for k = 0, SCRIPTS_COUNT - 1 do
        local script = string.unpack("<I4", slots, k * 4 + 1)
        if valid(script) then
            local from = script - NEAR
            local raw = mem.read(from, NEAR * 2)
            local at = 1
            while raw do
                local i = raw:find(TEXT_VALUE, at, true)
                if not i then break end
                local address = from + i - 1 - 8
                if i > 8 and not out[address] then
                    local units, chars = read_text(address)
                    if units and japanese(units) then out[address] = { units = units, raw = chars } end
                end
                at = i + 4
            end
        end
    end
    return out
end

-- The texts the scripts were at lately: by address { units, raw, when }.
local recent = {}
local last_key           -- the text reported last
local steady_key, steady = nil, 0
local resolved = false   -- whether the pictures on the screen were dealt with
local layouts = {}

local function drop()
    for name in pairs(layouts) do reader.layout(name, nil) end
    layouts = {}
end

reader.on_state_loaded(function()
    drop()
    recent, learnt, drawn_as, blank, last_key, steady_key, resolved = {}, {}, {}, nil, nil, nil, false
end)

-- What the game draws of a text: its characters with "\n" between rows; for a choice the
-- answers that follow the first one, each a row.
local function drawn_of(address, units)
    local drawn, parsed = {}, nil
    if units[1] == ANSWER_MARK then
        local answers = {}
        for k = 0, ANSWERS_MAX - 1 do
            local more = read_text(address - k * 2 * VALUE_SIZE)
            if not more or more[1] ~= ANSWER_MARK then break end
            answers[#answers + 1] = parse(more).rows[1]
        end
        for index, row in ipairs(answers) do
            if index > 1 then drawn[#drawn + 1] = "\n" end
            for _, ch in ipairs(row) do
                if ch ~= SPACE then drawn[#drawn + 1] = ch end
            end
        end
        return drawn, nil, answers
    end
    parsed = parse(units)
    for index, row in ipairs(parsed.rows) do
        if index > 1 then drawn[#drawn + 1] = "\n" end
        for _, ch in ipairs(row) do
            if ch ~= SPACE then drawn[#drawn + 1] = ch end
        end
    end
    return drawn, parsed, nil
end

reader.on_frame(function()
    local frame = reader.frame()
    -- Several scripts run at once (one is the scene's); the texts they are at are kept.
    local slots = mem.read(SCRIPTS, SCRIPTS_COUNT * 4)
    if slots and #slots == SCRIPTS_COUNT * 4 then
        for k = 0, SCRIPTS_COUNT - 1 do
            local script = string.unpack("<I4", slots, k * 4 + 1)
            if valid(script) then
                local address = script + AT
                local known = recent[address]
                if known then
                    known.when = frame
                else
                    local units, raw = read_text(address)
                    if units and japanese(units) then
                        recent[address] = { units = units, raw = raw, when = frame }
                    end
                end
            end
        end
    end
    if frame % EVERY ~= 0 then return end
    for address, known in pairs(recent) do
        if frame - known.when > RECENT then recent[address] = nil end
    end

    local glyphs = drawn_glyphs()
    if not glyphs or #glyphs == 0 then
        if next(layouts) then drop() end
        steady_key, resolved = nil, false
        return
    end
    local parts = {}
    for k, g in ipairs(glyphs) do parts[k] = g[1] * 1024 + g[2] end
    local key = table.concat(parts, ",")
    if key ~= steady_key then
        -- Other pictures: the places given are no longer true.
        if next(layouts) then drop() end
        steady_key, steady, resolved = key, 1, false
        return
    end
    steady = steady + 1
    -- Looked at when the pictures have just stopped, and once more when they have stood for
    -- longer than any wait inside a message.
    if resolved or (steady ~= STEADY and steady ~= STOOD) then return end
    local stood = steady == STOOD

    -- The pictures have stopped coming (or pause: a message is typed with waits): which of
    -- the scene's texts is it? One that fits and that a script was at lately, or of whose
    -- pictures half are known to be its characters and none known as another; of those the
    -- one with most pictures known.
    -- A message begins a row, after a name (three pictures) or without one; the window may
    -- still have the message before it in the rows above.
    local starts, named_start = {}, {}
    for k, g in ipairs(glyphs) do
        if k == 1 or glyphs[k - 1][2] ~= g[2] then
            starts[#starts + 1] = k
            if glyphs[k + NAME_GLYPHS] and glyphs[k + NAME_GLYPHS][2] == g[2] then
                starts[#starts + 1] = k + NAME_GLYPHS
                named_start[k + NAME_GLYPHS] = true
            end
        end
    end
    local best, tie
    local rival = -1       -- how well a longer text fits what is there so far
    local alone, several   -- the one text that is all there, if it is one
    for address, one in pairs(scene_texts(slots)) do
        local drawn, parsed, answers = drawn_of(address, one.units)
        local lately = recent[address] and frame - recent[address].when <= LATELY
        for _, start in ipairs(starts) do
            local ok, complete, count, agree, differ = fits(drawn, glyphs, start)
            if ok and not complete then
                if differ == 0 and agree > rival then rival = agree end
            elseif ok then
                if differ == 0 and count >= ALONE_MIN then
                    if alone and alone.raw ~= one.raw then
                        several = true
                    else
                        alone = { address = address, raw = one.raw, drawn = drawn, parsed = parsed, answers = answers,
                                  count = count, agree = agree, differ = 0, named = named_start[start], lately = 0 }
                    end
                end
                local sure = differ == 0 and count >= 3 and agree * 2 >= count
                local pointed = lately and count >= 6 and differ * 4 <= count
                if sure or pointed then
                    local this = { address = address, raw = one.raw, drawn = drawn, parsed = parsed, answers = answers,
                                   count = count, agree = agree, differ = differ, named = named_start[start],
                                   lately = lately and 1 or 0 }
                    local function better(x, y)
                        if (x.differ == 0) ~= (y.differ == 0) then return x.differ == 0 end
                        if x.agree ~= y.agree then return x.agree > y.agree end
                        if x.lately ~= y.lately then return x.lately > y.lately end
                        return nil
                    end
                    if not best then
                        best, tie = this, false
                    else
                        local verdict = better(this, best)
                        if verdict == true then
                            best, tie = this, false
                        elseif verdict == nil and this.raw ~= best.raw then
                            tie = true
                        end
                    end
                end
            end
        end
    end
    -- A longer text that fits what is there as well: the message may still be coming.
    if best and best.lately == 0 and rival >= best.agree then best = nil end
    -- Nothing known yet of these pictures, but they have stood, and one text only is all
    -- there: that is it (this is how the first messages after a start are made out).
    if (not best or tie) and stood and alone and not several then best, tie = alone, false end
    -- Two texts that fit alike: not known which (it is, once more pictures are known).
    if not best or tie then return end
    do
        local at = #glyphs - best.count
        for _, ch in ipairs(best.drawn) do
            if ch ~= "\n" then
                at = at + 1
                local id = glyphs[at][3]
                if learnt[id] and learnt[id] ~= ch then drawn_as[learnt[id]] = nil end
                if drawn_as[ch] and drawn_as[ch] ~= id then learnt[drawn_as[ch]] = nil end
                learnt[id], drawn_as[ch] = ch, id
            end
        end
    end
    resolved = true
    local text_key = text.hash(best.raw) .. ":" .. string.format("%x", best.address)
    local whole, ruby, slots_, window, channel
    if best.answers then
        whole, slots_ = compose_choice(best.answers)
        window, channel = "choice", "choice"
    else
        whole, ruby, slots_ = compose_message(best.parsed)
        window = "message"
    end
    if #whole == 0 then return end
    if text_key ~= last_key then
        last_key = text_key
        drop()
        -- The name in front of the message, when its pictures are known: a blank one is in
        -- the middle of a name of two characters and on both sides of a name of one.
        local speaker
        if best.named and not best.answers then
            local first = #glyphs - best.count - NAME_GLYPHS + 1
            local a, b, c = glyphs[first][3], glyphs[first + 1][3], glyphs[first + 2][3]
            if a == c then
                blank = a
            elseif learnt[a] and learnt[c] and not learnt[b] then
                blank = b
            end
            local name = {}
            for _, id in ipairs({ a, b, c }) do
                if id ~= blank then
                    if not learnt[id] then name = nil break end
                    name[#name + 1] = learnt[id]
                end
            end
            speaker = name and #name > 0 and table.concat(name) or nil
        end
        reader.emit{ text = whole, speaker = speaker, ruby = ruby, channel = channel, window = window, id = text_key }
    end
    local cells, at = {}, #glyphs - best.count
    for _, slot in ipairs(slots_) do
        if slot == "drawn" then
            at = at + 1
            cells[#cells + 1] = { glyphs[at][1], glyphs[at][2], GLYPH_W - 1, CELL_HEIGHT }
        elseif slot == "mark" then
            at = at + 1
        else
            cells[#cells + 1] = false
        end
    end
    reader.layout(window, { space = { 640, 448 }, glyphs = cells })
    layouts[window] = true
end)
