-- user config for rand-sig.lua:
minargs      = 0
maxargs      = 16
minaggrfields= 0    -- 0 allows for empty structs/unions
maxaggrfields= 12
maxarraylen  = 16   -- see *)
arraydice    = 40   -- how often to turn a member into an array (1 out of arraydice-times)
maxaggrdepth = 3    -- max nesting depth of aggregates, 1 = no nesting
reqaggrinsig = true -- require that every generated signature has at least one aggregate
ncases       = 400
types        = "BcsijlCSIJLpfd{}<>"  -- types to use; use '{','}' for structs, '<','>' for unions
rtypes       = nil                   -- supported return types (set to nil to use "v"..types)
seed         = 2112

-- *) note some callconvs pass structs via regs, so using big numbers here will
--    reduce those cases; however special alignment rules are specified in some
--    ABIs for bigger arrays, which is also worth testing



-- Notes: specify types more than once to increase relative occurance, e.g.:

-- this favors non-aggregate args, especially ints (and also increases avg num of aggregate fields):
--types       = "Bccssiiiiijjllpfd{}"

-- this heavily favors nested structs, while not having any union:
--types       = "Bcsijlpfd{{{{{{}"

-- this heavily favors flat and short/empty aggregates:
--types       = "Bcsijlpfd{}}}}}}<>>>>>"



-- user config for mk-cases.lua

-- if true, aggregates are copied via = operator, otherwise field by field
aggrcpsimple = true

-- if true, aggregate params are modified in called function, to test whether
-- they are passed "by value" (important for verifying by-value semantics for
-- callconvs where hidden pointers are used instead of real copies; aggregate
-- passing implementations must use of temporary copies in those cases)
aggrmutabletest = true

-- force aggregate packing, 0=off, pos values set fixed packing, neg values
-- set a random power-of-2 packing per aggregate, within [1,abs(aggrpacking)]
aggrpacking = 0
aggrpackingseed = seed

