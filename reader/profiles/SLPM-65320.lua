-- reader: no hooks
-- reader: title EVE burst error PLUS
-- reader: reads The message window: what is said and told, part by part as it comes up in full
-- reader: version SLPM-65320, disc version 1.02
-- EVE burst error PLUS (Game Village, PS2, 2003), SLPM-65320. Program 8EC6655D.
--
-- Plain Shift-JIS. A message has the speaker in 【】 at its front and `s` at each wait. The
-- game copies it into a buffer (parts separated by NUL) and keeps a text object for the
-- window: the count of characters shown and the characters, one 32-bit word each (Shift-JIS
-- code, bytes swapped). A word beside it is 1 while the window waits for the button.
--
-- The object has no positions. Characters are laid out 23 to a row, later rows indented by
-- one, in a 640x448 picture (measured).

local BUILD = "8EC6655D"
if reader.build_id ~= BUILD then
    error("this profile is for EVE burst error PLUS SLPM-65320 (program " .. BUILD ..
          "), the game is " .. tostring(reader.build_id))
end

local WAITING = 0x0021a110   -- 1 while the window waits for the button (and a voice is over)
local MESSAGE = 0x0049b590   -- the message: its parts in Shift-JIS, a NUL after each
local MESSAGE_MAX = 0x400
local OBJECT = 0x0021a118    -- pointer to the window's text object
local O_COUNT, O_CHARS = 0x30, 0x50
local MAX_CHARS = 400

local COLUMNS = 23
local LEFT, TOP, PITCH, ROW, HEIGHT = 16, 325, 25, 35.6, 26

-- The characters up in the window: a list of { text, half }.
local function characters()
    local object = mem.u32(OBJECT)
    if not object or object < 0x100000 or object >= 0x2000000 then return nil end
    local count = mem.u32(object + O_COUNT)
    if not count or count == 0 or count > MAX_CHARS then return nil end
    local raw = mem.read(object + O_CHARS, count * 4)
    if not raw or #raw < count * 4 then return nil end
    local out = {}
    for k = 0, count - 1 do
        local code = string.unpack("<I4", raw, k * 4 + 1) & 0xffff
        local bytes = code > 0xff and string.char(code >> 8, code & 0xff) or string.char(code)
        out[#out + 1] = { text = text.sjis(bytes), half = code <= 0xff }
    end
    return out
end

-- How many characters the message has up to the end of each of its parts: { [count] = true }.
local function part_ends()
    local raw = mem.read(MESSAGE, MESSAGE_MAX)
    if not raw then return {} end
    local out, count, at = {}, 0, 1
    while at <= #raw do
        local b = raw:byte(at)
        if b == 0 then
            if count == 0 or raw:byte(at - 1) == 0 then break end
            out[count] = true
            at = at + 1
        elseif (b >= 0x81 and b <= 0x9f) or (b >= 0xe0 and b <= 0xfc) then
            count, at = count + 1, at + 2
        elseif b == 0x73 and raw:byte(at + 1) == 0 then
            at = at + 1     -- the wait, which the script has before the NUL
        else
            count, at = count + 1, at + 1
        end
    end
    return out
end

-- Row breaking as in the game: a 、 or 。 that would be 24th hangs at the row's end; a
-- character that may not begin a row takes the one before it to the next row (measured).
local HANGING = { ["、"] = true, ["。"] = true, ["，"] = true, ["．"] = true }
local NO_START = {}
for c in ("、。，．・：；？！ー‥…」』）］】》〉｝ぁぃぅぇぉっゃゅょゎァィゥェォッャュョヮヵヶ々"):gmatch(utf8.charpattern) do
    NO_START[c] = true
end

local function places(chars)
    local columns = {}   -- the column of each character, and its row
    local rows = {}
    local row, column, row_start = 0, 0, 1
    local k = 1
    while k <= #chars do
        local c = chars[k]
        local width = c.half and 0.5 or 1
        if column + width > COLUMNS + 0.01 then
            if HANGING[c.text] and column <= COLUMNS + 0.01 then
                columns[k], rows[k] = column, row
                row, column, row_start, k = row + 1, 1, k + 1, k + 1
                goto continue
            end
            -- Back to a character that may begin a row, but never past the row's own start.
            local first = k
            while first > row_start + 1 and NO_START[chars[first].text] do first = first - 1 end
            row, column, row_start, k = row + 1, 1, first, first
            goto continue
        end
        columns[k], rows[k] = column, row
        column = column + width
        k = k + 1
        ::continue::
    end
    local out = {}
    for k2, c in ipairs(chars) do
        local width = c.half and 0.5 or 1
        out[k2] = { LEFT + columns[k2] * PITCH, TOP + rows[k2] * ROW, width * PITCH, HEIGHT }
    end
    return out
end

local reported = 0      -- how many of the characters up have been reported
local last_key          -- the characters up when last read
local serial = 0
local speaker           -- the message's speaker, for its later parts too
local parts = {}        -- the window's parts reported: { name, first, last }

local function drop_all()
    for _, part in ipairs(parts) do reader.layout(part.name, nil) end
    parts = {}
end

reader.on_state_loaded(function()
    drop_all()
    reported, last_key = 0, nil
end)

reader.on_frame(function()
    local chars = characters()
    if not chars then
        if #parts > 0 then drop_all() end
        reported, last_key = 0, nil
        return
    end
    local texts = {}
    for k, c in ipairs(chars) do texts[k] = c.text end
    local key = table.concat(texts)
    -- A new message: what is up no longer begins with what was reported.
    if last_key and key:sub(1, #last_key) ~= last_key then
        drop_all()
        reported = 0
    end
    last_key = key
    -- A part is there when the game waits for the button, or (while a voice still speaks)
    -- when as many characters are up as the message has to the end of a part.
    if #chars <= reported then return end
    if mem.u32(WAITING) ~= 1 and not part_ends()[#chars] then return end

    local cells = places(chars)
    local first = reported + 1
    -- The speaker's name in 【】 at the message's front is not part of what is said.
    if first == 1 then speaker = nil end
    if first == 1 and chars[1].text == "【" then
        for k = 2, math.min(#chars, 16) do
            if chars[k].text == "】" then
                speaker = table.concat(texts, "", 2, k - 1)
                first = k + 1
                break
            end
        end
    end
    local b, glyphs = reader.builder(), {}
    for k = first, #chars do
        b:add(chars[k].text)
        glyphs[#glyphs + 1] = cells[k]
    end
    local whole = b:text()
    reported = #chars
    if whole == "" then return end
    serial = serial + 1
    local name = "message" .. (serial % 8)
    reader.emit{ text = whole, speaker = speaker, window = name,
                 id = text.hash(key) .. ":" .. serial }
    reader.layout(name, { space = { 640, 448 }, glyphs = glyphs })
    parts[#parts + 1] = { name = name }
end)
