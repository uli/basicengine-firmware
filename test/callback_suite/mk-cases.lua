require "config"

-- use shared helpers to generate cases
package.path = '../common/?.lua;' .. package.path
require"mk-cases"


function mkcase(id,sig)
  local sig = trim(sig)
  local rtype = string.sub(sig, -1)
  local i = 1
  local args = { rtype }
  while i < #sig do
    c = string.sub(sig, i, i)
    if(c == ')') then
      break
    end
    if(c == '_') then -- filter out prefixes
      i = i + 1
    else
      args[#args+1] = c
    end
    i = i + 1
  end
  return "F" .. (#args-1) .. "(f" .. id .. "," .. table.concat(args,',') .. ")\n"
end

function mkall()
  -- case macros
  for i = minargs, maxargs do
    local line = "#define F" .. i .. "(ID,R"
    local argdef = { }
    local argset = { }
    if i > 0 then
      line = line .. ","
      for j = 0, i-1 do
        argdef[#argdef+1] = "M" .. j
        argset[#argset+1] = "K_##M" .. j .. "[" .. j .. "]"
      end
    end
    line = line .. table.concat(argdef,",") .. ") void ID(void* addr) { write_V_##R(" .. i .. ", ((" .. api .. " R(*)("  .. table.concat(argdef,",") .. "))addr)(" .. table.concat(argset,",") .. "));}\n"
    io.write(line)
  end

  -- cases
  local lineno = 0
  local sigtab = { }
  local cases = ''
  for line in io.lines() do
    local sig = trim(line)
    cases = cases..mkcase(lineno,sig)
    sigtab[#sigtab+1] = sig
    lineno = lineno + 1
  end

  io.write(cases)
  io.write(mkfuntab(lineno, 'f', 'funptr', 'G_funtab', false))
  io.write(mksigtab(sigtab, ccprefix, 'G_sigtab'))
  io.write("int G_maxargs = "..maxargs..";\n")
end

mkall()

