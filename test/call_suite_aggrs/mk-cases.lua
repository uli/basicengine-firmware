require"config"

-- use shared helpers to generate cases
package.path = '../common/?.lua;' .. package.path
require"mk-cases"



-- returns: generated case str, num args; accumulates unique idx => aggr-sig in
-- aggrs (sequentially) and aggr-sig => {body,name} in seen_aggrs (depth first
-- for nested aggrs, so sub-aggrs conveniently precede parents)
function mkcase(id, sig, aggrs, seen_aggrs)
  local sig = trim(sig)
  local fsig = put_sig_rtype_first(sig)
  local h = { "/* ",id,":",sig," */ " }
  local t = { }
  local pos = -1
  local n_nest = 0
  local aggr = { }
  local aggr_sig = { }
  aggr[0] = { }     -- non-sequential [0] collects all non-aggr types (not used, though)
  aggr_sig[0] = ''
  for i = 1, #fsig do
    local name = "a"..pos
    local ch   = fsig:sub(i,i)

    -- aggregate nest level change?
    if ch == '{' or ch == '<' then
      n_nest = n_nest + 1
      aggr[n_nest] = { }
      aggr_sig[n_nest] = ''
    end

    aggr_sig[n_nest] = aggr_sig[n_nest]..ch

    -- array? Just append to name of member var from prev loop
    if ch:match('[%[%]0123456789]') ~= nil then
      aggr[n_nest][#aggr[n_nest]] = aggr[n_nest][#aggr[n_nest]]..ch
    else
      -- register (sub)aggrs on closing char
      if ch == '}' or ch == '>' then
        -- only add unseen aggregates, key is aggr sig, val is body and name
        if seen_aggrs[aggr_sig[n_nest]] == nil then
          aggrs[#aggrs+1] = aggr_sig[n_nest]
          if ch == '}' then ch = 'struct ' else ch = 'union ' end
          ch = ch..'A'..#aggrs
          seen_aggrs[aggr_sig[n_nest]] = { aggr[n_nest], ch }
        end
        ch = seen_aggrs[aggr_sig[n_nest]][2]

        n_nest = n_nest - 1
        aggr_sig[n_nest] = aggr_sig[n_nest]..aggr_sig[n_nest+1]
      end

      -- add member type and var name to aggr
      if ch ~= '{' and ch ~= '}' and ch ~= '<' and ch ~= '>' then
        aggr[n_nest][#aggr[n_nest]+1] = ch
        aggr[n_nest][#aggr[n_nest]+1] = 'm'..(#aggr[n_nest] >> 1)
      end

      -- no nesting (= actual func args), generate case code
      if n_nest == 0 then
        h[#h+1] = ch
        -- aggregate types have more than one char
        if #ch > 1 then
          if aggrcpsimple then
            t[#t+1] = '*('..ch..'*)V_a['..pos.."]="..name..";"
          else
            t[#t+1] = 'f_cp'..ch:match('A.*')..'(V_a['..pos.."],&"..name..");"
          end
          if aggrmutabletest then
            t[#t] = t[#t]..'memset(&'..name..',0,sizeof('..name..'));'
          end
        else
          t[#t+1] = "V_"..ch.."["..pos.."]="..name..";"
        end

        -- is return type or func arg?
        if pos == -1 then
          h[#h+1] = " f"..id.."("
          h[#h+1] = ''
          t[#t] = ''  -- clear; aggr return type handled explicitly
        else
          h[#h+1] = ' '..name
          h[#h+1] = ","
        end

        pos = pos + 1
      end
    end
  end
  h[#h] = "){"
  if #h[6] == 1 then
    t[#t+1] = "ret_"..h[6].."("..pos..")}\n"
  else
    t[#t+1] = "ret_a("..pos..","..h[6]..")}\n"
  end
  return table.concat(h,"")..table.concat(t,""), pos
end


function mkall()
  local lineno = 0
  local sigtab = { }
  local cases = ''
  local aggrs = { }
  local seen_aggrs = { }
  local max_numargs = 0

  for line in io.lines() do
    local sig = trim(line)
    local c, n = mkcase(lineno, sig, aggrs, seen_aggrs)
    cases = cases..c
    max_numargs = math.max(max_numargs, n)
    sigtab[#sigtab+1] = sig
    lineno = lineno + 1
  end

  local agg_defs, agg_sizes, agg_sigs, agg_names = mkaggrdefs(aggrs, seen_aggrs, aggrpacking, aggrpackingseed, aggrcpsimple)

  -- make table.concat work
  if #agg_names > 0 then
    table.insert(agg_names, 1, '')
  end

  io.write(table.concat(agg_defs,'\n')..'\n')
  io.write(cases)
  io.write(mkfuntab(lineno, 'f', 'funptr', 'G_funtab', true))
  io.write(mksigtab(sigtab, '', 'G_sigtab'))
  io.write('const char* G_agg_sigs[]  = {\n\t"'..table.concat(agg_sigs, '",\n\t"')..'"\n};\n')
  io.write('int G_agg_sizes[] = {\n\t'..table.concat(agg_sizes, ',\n\t')..'\n};\n')
  io.write('funptr G_agg_touchAfuncs[] = {'..string.sub(table.concat(agg_names, ',\n\t(funptr)&f_touch'),2)..'\n};\n')
  io.write('funptr G_agg_cmpfuncs[] = {'..string.sub(table.concat(agg_names, ',\n\t(funptr)&f_cmp'),2)..'\n};\n')
  io.write("int G_maxargs = "..max_numargs..";\n")
end

mkall()

