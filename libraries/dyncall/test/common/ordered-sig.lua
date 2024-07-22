-- !does not generates sigs with aggregates!

-- following knobs control generation:

-- required to be defined by who is using this:
-- ncases
-- types

-- optional:
-- rtypes (if not set, it'll be 'v'..types)


--------------------------------

if rtypes == nil then
  rtypes = "v"..types
end

local i
for i = 0, ncases-1 do
  local s = ""
  local typeindex
  local ntypes = #types
  local nrtypes = #rtypes
  local x = offset+i*step
  if x >= nrtypes then
    local y =  math.floor(x / nrtypes) - 1
    while y >= ntypes do
      typeindex = 1 + (y % ntypes)
      s = s .. string.sub(types, typeindex, typeindex)
      y = math.floor(y / ntypes) - 1
    end
    typeindex = 1 + (y % ntypes)
    s = s .. string.sub(types, typeindex, typeindex)
  end
  typeindex = 1 + (x % nrtypes)
  io.write(s .. ")" .. string.sub(rtypes, typeindex, typeindex) .. "\n")
end

