function trim(s)
  return s:gsub("^%s+",""):gsub("%s+$","")
end


-- parse array notation, e.g. returns "a", 4 for "a[4]"
function split_array_decl(s)
  local name = s
  local n = nil  -- not an array
  local i = s:find('%[')
  if i ~= nil then
    name = name:sub(1, i-1)
    n = tonumber(s:sub(i):match('[0123456789]+'))
  end
  return name, n
end


-- returns sig with return type first, and no ')' separator, e.g.:
-- 'ijf)v' -> 'vijf'
-- '){ii}' -> '{ii}'
function put_sig_rtype_first(sig)
  return sig:sub(sig:find(')')+1,-1)..sig:sub(1,sig:find(')')-1)
end


-- aggrs: (sequential) idx => aggr-sig
-- seen_aggrs: aggr-sig => {{type0, name0, ...}, aggr_name}
-- packing: 0=off, pos values set fixed packing, neg values set a random
--          power-of-2 packing per aggregate, within [1,abs(aggrpacking)]
-- packing_seed: seed for random packing (if used)
-- cpsimple: whether or not to copy aggregates via '=' or field by field
function mkaggrdefs(aggrs, seen_aggrs, packing, packingseed)
  local agg_defs  = { }
  local agg_sizes = { }
  local agg_sigs  = { }
  local agg_names = { }

  math.randomseed(packingseed)

  for a = 1, #aggrs do
    local k = aggrs[a]
    local v = seen_aggrs[k]
    local am = v[1]            -- aggregate members
    local at = v[2]            -- aggregate type
    local an = at:match('A.*') -- aggregate name (w/o struct or union)

    -- aggregate def
    aggr_def = '/* '..k..' */\n'
    if packing ~= 0 then
      local pack = packing
      if pack < 0 then
        pack = math.floor(math.pow(2,math.floor(math.log(math.random(math.abs(pack)),2))))
      end
      aggr_def = aggr_def..'#pragma pack(push,'..pack..')\n'
    end

    aggr_def = aggr_def..at..' { '
    for i = 1, #am, 2 do
      aggr_def = aggr_def..am[i]..' '..am[i+1]..'; '
    end
    aggr_def = aggr_def..'};\n'

    if packing ~= 0 then
      aggr_def = aggr_def..'#pragma pack(pop)\n'
    end

    -- aggregate cp and cmp funcs
    s = {
      'void f_cp'..an..'('..at..' *x, const '..at..' *y) { ',
      'int f_cmp'..an..'(const '..at..' *x, const '..at..' *y) { return '
    }
    o = { '=', '==', 'f_cp', 'f_cmp', '; ', ' && ', '', '1' }
    for t = 1, 2 do
      if t ~= 1 or cpsimple == false then
        aggr_def = aggr_def..s[t]
        local b = {}
        for i = 1, #am, 2 do
          local mn, mc = split_array_decl(am[i+1]) -- aggr member name and (array) count
          local fmt = ''
          if mc ~= nil then -- need array suffixes?
            fmt = '[%d]'
          else
            mc = 1
          end

          for j = 1, mc do
            name = mn..fmt:format(j-1)
            amn = am[i]:match('A.*')
            if amn then -- is aggr?
              b[#b+1] = o[t+2]..amn..'(&x->'..name..', &y->'..name..')'
            else
              b[#b+1] = 'x->'..name..' '..o[t]..' y->'..name
            end
          end
        end
        if #b == 0 then  -- to handle empty aggregates
          b[1] = o[t+6]
        end
        aggr_def = aggr_def..table.concat(b,o[t+4])..'; };\n'
      end
    end

    -- write convenient dcNewAggr() helper/wrapper funcs
    aggr_def = aggr_def..'DCaggr* f_touch'..an..'() {\n\tstatic DCaggr* a = NULL;\n\tif(!a) {\n\t\ta = dcNewAggr('..(#am>>1)..', sizeof('..at..'));\n\t\t'
    for i = 1, #am, 2 do
      local mn, mc = split_array_decl(am[i+1])
      if mc == nil then
        mc = 1
      end
      amn = am[i]:match('A.*')
      if amn then -- is aggr?
        --aggr_def = aggr_def..'dcAggrField(at, DC_SIGCHAR_AGGREGATE, offsetof('..at..', '..mn..'), '..mc..', f_touch'..amn..'());\n\t\t'
        aggr_def = aggr_def.."AFa("..at..','..mn..','..mc..','..amn..')\n\t\t'
      else
        --aggr_def = aggr_def.."dcAggrField(at, '"..am[i].."', offsetof("..at..', '..mn..'), '..mc..');\n\t\t'
        aggr_def = aggr_def.."AF('"..am[i].."',"..at..','..mn..','..mc..')\n\t\t'
      end
    end

    agg_defs [#agg_defs  + 1] = aggr_def..'dcCloseAggr(a);\n\t}\n\treturn a;\n};'
    agg_sizes[#agg_sizes + 1] = 'sizeof('..at..')'
    agg_sigs [#agg_sigs  + 1] = k
    agg_names[#agg_names + 1] = an
  end

  return agg_defs, agg_sizes, agg_sigs, agg_names
end


function mkfuntab(n, prefix, t, array_name, with_cast)
  local s = { t.." "..array_name.."[] = {\n"}
  local cast = ''
  if with_cast == true then
    cast = '('..t..')'
  end
  for i = 0, n-1 do
    s[#s+1] = "\t"..cast.."&"..prefix..i..",\n"
  end
  s[#s+1] = "};\n"
  return table.concat(s,"")
end


-- @@@ sigprefix should be added by generators, not here
function mksigtab(sigs, sigprefix, array_name)
  local s = { "const char * "..array_name.."[] = {\n"}
  for k,v in pairs(sigs) do
    s[#s+1] = '\t"'..sigprefix..v..'",\n'
  end
  s[#s+1] = "};\n"
  return table.concat(s,"")
end

