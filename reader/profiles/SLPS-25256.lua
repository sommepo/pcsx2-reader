-- reader: no hooks
-- reader: title Never7 -the end of infinity-
-- reader: reads The story's text with speakers, each message as it has come up in full, and choices
-- reader: version SLPS-25256, disc version 1.01
-- Never7 -the end of infinity- (KID, PS2, 2003), SLPS-25256. Program 9D260CAF.
--
-- The game keeps the rows of text it has shown, as it broke them, in a list: a row is added
-- when it has been typed out. When the list is full (999 rows, some time into the first
-- morning) the count stays and every row moves up one place for each new one. A speaker's name is a row of its own that begins with a space.
-- Which row of the text window the last one stands in is a number of the program's, so the
-- rows before it are the ones above. The game has no table of where the characters are; it
-- draws them at a fixed pitch, brackets and commas at half of it (measured in pictures of
-- the game). A choice's answers are in buffers of their own and are drawn the same way,
-- each row centred.

local BUILD = "9D260CAF"
if reader.build_id ~= BUILD then
    error("this profile is for Never7 SLPS-25256 (program " .. BUILD ..
          "), the game is " .. tostring(reader.build_id))
end

-- Data of the program (it is loaded where it was linked for).
local ROWS = 0x00599b34        -- the rows shown so far, Shift-JIS, each in a buffer of its own
local ROW_SIZE = 100
local ROW_COUNT = 0x00398280   -- how many rows there are
local SETTLED = 0x00397f78     -- 1 when the message has been typed out, 0 while it is
local WINDOW_ROW = 0x00397f7c  -- the row of the text window the last row stands in (from 0)
local ANSWER_TEXT = 0x005cbe57 -- a choice's answers, each in a buffer of its own
local ANSWER_SIZE = 66
local ANSWER_MAX = 6
local ANSWER_COUNT = 0x00398630 -- how many answers the choice has (kept afterwards)
local CHOICE_BOX = 0x00397f4c  -- not 0 while a choice's box is up
local MIDDLE_X, MIDDLE_Y = 320, 196 -- the answers stand around this point of the picture

-- Where the text stands in the picture (640 by 448).
local LEFT, TOP = 24, 319
local PITCH, ROW_PITCH, HEIGHT = 24, 28, 26
local WIDTH = 576              -- a row is this wide at most
local HALF = {}                -- characters half as wide as the others
for _, c in ipairs{ "、", "。", "（", "）", "「", "」", "『", "』" } do HALF[c] = true end
-- A row that ends in one of these ends what is said there; any other goes on in the next.
local ENDS = {}
for _, c in ipairs{ "。", "」", "』", "）", "！", "？", "…", "―", "♪", "～" } do ENDS[c] = true end

-- A message is reported when it has been all there for this many frames: what a skip runs
-- through is not read.
local SHOWN_FRAMES = 4
-- The text window has this many rows: rows further back than that are no longer shown.
local WINDOW_ROWS = 4

local function is_lead(b)
    return (b >= 0x81 and b <= 0x9f) or (b >= 0xe0 and b <= 0xfc)
end

-- A row's characters and how wide each is.
local function characters(raw)
    local out, at = {}, 1
    while at <= #raw do
        local b = raw:byte(at)
        if is_lead(b) and at < #raw then
            local c = text.sjis(raw:sub(at, at + 1))
            out[#out + 1] = { c, HALF[c] and PITCH / 2 or PITCH }
            at = at + 2
        else
            out[#out + 1] = { string.char(b), PITCH / 2 }
            at = at + 1
        end
    end
    return out
end

local function row(index)
    local raw = mem.read(ROWS + index * ROW_SIZE, ROW_SIZE)
    return raw and raw:match("^[^%z]*") or ""
end

local function blank(raw)
    return raw == "" or raw == " "
end

local reported = nil  -- how many rows had been there when text was last reported
local tail            -- rows before that (not the last, which may still grow), to follow them
local TAIL = 3        -- when the full list moves up
local last_row, last_size -- the last row reported, and how many bytes it had then
local settled_since
local windows = {}
local answers_shown

-- `n` rows that end just before row `to`.
local function rows_before(to, n)
    local out = {}
    for index = to - n, to - 1 do out[#out + 1] = index >= 0 and row(index) or "" end
    return out
end

local function same(a, b)
    for index = 1, #a do
        if a[index] ~= b[index] then return false end
    end
    return true
end

local function withdraw()
    for name in pairs(windows) do reader.layout(name, nil) end
    windows = {}
end

-- Whether a choice is up: the box the answers stand in is. (A bit of the program's state
-- that is on at a choice is on at the end of many a message as well, and is off when a
-- quiz asks again with the wrong answers taken away.)
local function choosing()
    return (mem.u32(CHOICE_BOX) or 0) ~= 0
end

-- The answers of a choice: one event, an answer to a line.
local function choice()
    local raw = mem.read(ANSWER_TEXT, ANSWER_SIZE * ANSWER_MAX)
    if not raw then return end
    local rows = {}
    -- The answers stay in their buffers after the choice, and those of an earlier choice
    -- with more answers after the last: the box says when one is up, the count how many.
    local wanted = choosing() and math.min(mem.u32(ANSWER_COUNT) or 0, ANSWER_MAX) or 0
    for index = 1, wanted do
        local one = raw:sub((index - 1) * ANSWER_SIZE + 1, index * ANSWER_SIZE):match("^[^%z]*")
        if one == "" then break end
        rows[#rows + 1] = one
    end
    local key = table.concat(rows, "\n")
    if #rows < 2 then
        if answers_shown then
            reader.layout("choice", nil)
            answers_shown = nil
        end
        return
    end
    if key == answers_shown then return end
    answers_shown = key
    local b, cells = reader.builder(), {}
    for index, one in ipairs(rows) do
        if index > 1 then
            b:add("\n")
            cells[#cells + 1] = false
        end
        local chars = characters(one)
        local width = 0
        for _, each in ipairs(chars) do width = width + each[2] end
        local x = MIDDLE_X - width / 2
        local y = MIDDLE_Y - #rows * ROW_PITCH / 2 + (index - 1) * ROW_PITCH
        for _, each in ipairs(chars) do
            b:add(each[1])
            cells[#cells + 1] = { x, y, each[2], HEIGHT }
            x = x + each[2]
        end
    end
    reader.emit{ text = b:text(), channel = "choice", window = "choice", id = "choice:" .. text.hash(key) }
    reader.layout("choice", { space = { 640, 448 }, glyphs = cells })
end

reader.on_frame(function(frame)
    choice()
    local count = mem.u32(ROW_COUNT)
    if not count or count > 100000 then return end
    if reported == nil or count < reported then
        -- The profile's first frame, or a game begun or loaded: what is there is not new.
        reported, last_row, last_size, tail = count, nil, nil, rows_before(count - 1, TAIL)
        withdraw()
        return
    end
    -- The full list moved up: where the rows reported last are now.
    if tail and not same(rows_before(reported - 1, TAIL), tail) then
        local moved
        for by = 1, math.min(reported - 1 - TAIL, 200) do
            if same(rows_before(reported - 1 - by, TAIL), tail) then
                moved = by
                break
            end
        end
        if not moved then
            -- Not found (a game loaded with as many rows): what is there is not new.
            reported, last_row, last_size, tail = count, nil, nil, rows_before(count - 1, TAIL)
            withdraw()
            return
        end
        reported = reported - moved
        if last_row then last_row = last_row - moved end
    end
    -- While a choice is up the message under it counts as typed out, which the game does not
    -- say then.
    if mem.u32(SETTLED) ~= 1 and not choosing() then
        settled_since = nil
        return
    end
    -- The last row with text (the game adds an empty one at times).
    local last = count - 1
    while last >= 0 and last >= count - 3 and blank(row(last)) do last = last - 1 end
    if last < 0 or blank(row(last)) then return end
    local grown = last == last_row and #row(last) > (last_size or 0)
    if count == reported and not grown then
        settled_since = nil
        return
    end
    settled_since = settled_since or frame
    if frame - settled_since < SHOWN_FRAMES then return end
    settled_since = nil

    -- New rows, or the last row gone on after a pause in the middle of it.
    local from, skip = reported, 0
    if grown then
        from, skip = last, last_size
    elseif last_row and from <= last_row then
        from = last_row + 1
    end
    if from > last then
        reported = count
        tail = rows_before(count - 1, TAIL)
        return
    end
    local at_row = mem.u32(WINDOW_ROW) or 0
    if at_row > 7 then at_row = 0 end
    -- Only what the window shows: rows that went by while the game skipped (or was hurried on
    -- to its next choice) are not read.
    if last - from >= WINDOW_ROWS then
        from, skip, grown = last - math.min(at_row, WINDOW_ROWS - 1), 0, false
    end
    if at_row - (last - from) <= 0 and not grown then
        withdraw() -- the window was cleared for these rows
    end

    local b, cells = reader.builder(), {}
    local speaker, ended, any
    for index = from, last do
        local raw = row(index)
        if index == from and skip > 0 then raw = raw:sub(skip + 1) end
        local name = raw:match("^ +(.-) *$")
        if name and name ~= "" and index < last then
            speaker = speaker or text.sjis(name)
        elseif not blank(raw) then
            if any and ended then
                b:add("\n")
                cells[#cells + 1] = false
            end
            local x = LEFT
            if index == from and skip > 0 then
                for _, each in ipairs(characters(row(index):sub(1, skip))) do x = x + each[2] end
            end
            local y = TOP + (at_row - (last - index)) * ROW_PITCH
            local chars = characters(raw)
            for _, each in ipairs(chars) do
                b:add(each[1])
                cells[#cells + 1] = { x, y, each[2], HEIGHT }
                x = x + each[2]
            end
            any = true
            ended = #chars > 0 and ENDS[chars[#chars][1]] or false
        end
    end
    reported, last_row, last_size = count, last, #row(last)
    tail = rows_before(count - 1, TAIL)
    local whole = b:text()
    if whole == "" then return end
    local window = "message" .. from .. (grown and "+" or "")
    reader.emit{ text = whole, speaker = speaker, window = window,
                 id = tostring(from) .. ":" .. skip .. ":" .. text.hash(whole) }
    reader.layout(window, { space = { 640, 448 }, glyphs = cells })
    windows[window] = true
end)
