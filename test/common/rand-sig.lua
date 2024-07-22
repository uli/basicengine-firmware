-- following knobs control generation:

-- required to be defined by who is using this:
-- minargs
-- maxargs
-- ncases
-- types
-- seed

-- optional:
-- rtypes (if not set, it'll be 'v'..types)
-- ellipsis

-- optional (when including aggregate generation):
-- minaggrfields
-- maxaggrfields
-- maxarraylen
-- arraydice
-- maxaggrdepth
-- reqaggrinsig


--------------------------------

if maxaggrdepth == nil then
  maxaggrdepth = 3
end


-- assure aggr chars are present in pairs (can be weighted, though), to avoid
-- inf loops; closing chars are allowed to appear alone, as they are ignored
-- without any opening char (does not make a lot of sense, though)
pairs_op = { '{', '<' } --, '[' }
pairs_cl = { '}', '>' } --, ']' }

for i = 1, #pairs_op do
  if string.find(types, '%'..pairs_op[i]) and not string.find(types, '%'..pairs_cl[i]) then
    types = types..pairs_cl[i]
  end
end


if rtypes == nil then
  rtypes = "v"..types
end


function mkaggr(n_nest, maxdepth, o, c)
  local s = o
  local nfields = 0

  repeat
    local t = c
    if nfields < maxaggrfields then
      repeat
        local id = math.random(#types)
        t = types:sub(id,id)
      until t ~= c or nfields >= minaggrfields
    end

    s_ = mktype(t, n_nest, maxdepth, o) or ''
    if(#s_ > 0) then
      nfields = nfields + 1
    end
    s = s..s_

    -- member (which cannot be first char) as array? Disallow multidimensional arrays
    if #s > 1 and t ~= c and s:sub(-1) ~= ']' and math.random(arraydice) == 1 then
      s = s..'['..math.random(maxarraylen)..']'
    end
  until t == c

  return s
end

function mktype(t, n_nest, maxdepth, aggr_open)
  -- aggregate opener?
  local aggr_i = 0
  for i = 1, #pairs_op do
    if pairs_op[i] == t then
      aggr_i = i
      break
    end
  end

  -- ignore new aggregates if above depth limit
  if aggr_i ~= 0 and t == pairs_op[aggr_i] then
    if n_nest < maxdepth then
      return mkaggr(n_nest + 1, maxdepth, pairs_op[aggr_i], pairs_cl[aggr_i])
    else
      return nil
    end
  end

  -- aggregate closer?
  for i = 1, #pairs_cl do
    if pairs_cl[i] == t then
      aggr_i = i
      break
    end
  end

  -- if closing char, without any open, ignore
  if aggr_i ~= 0 and (aggr_open == nil or pairs_op[aggr_i] ~= aggr_open) then
    return nil
  end

  return t
end

-- pattern matching aggregate start chars
local aggr_op_pattern = '[%'..table.concat(pairs_op,'%')..']'

math.randomseed(seed)
local id
local uniq_sigs = { }
for i = 1, ncases do
  local l = ''
  repeat
    local nargs = math.random(minargs,maxargs)
    local varargstart = (ellipsis and nargs > 0 and math.random(0,ellipsis) == 0) and math.random(0,nargs-1) or nargs -- generate vararg sigs?
    local sig = { }
    for j = 1, nargs do
      id = math.random(#types)
      sig[#sig+1] = mktype(types:sub(id,id), 0, math.random(maxaggrdepth), nil) -- random depth avoids excessive nesting
      -- start vararg part?
      if j > varargstart and #sig > 0 then
        sig[#sig+1] = "."
        varargstart = nargs
      end
    end
    repeat
      id = math.random(#rtypes)
      r = mktype(rtypes:sub(id,id), 0, math.random(maxaggrdepth), nil) -- random depth avoids excessive nesting
    until r
    sig[#sig+1] = ')'..r
    l = table.concat(sig)
    -- reject dupes, sigs without any aggregate (as this is about aggrs after all), and empty ones (if not wanted)
  until (reqaggrinsig ~= true or string.match(l, aggr_op_pattern) ~= nil) and uniq_sigs[l] == nil
  uniq_sigs[l] = 1

  io.write(l.."\n")
end

