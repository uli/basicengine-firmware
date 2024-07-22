-- auto-generated file from template config-random.lua (created by make)
-- section 'main' --------------------------------------------------------------
                  
ncases    = 800         -- number of test cases (note: inf loop if higher than max arg permutation)
mode      = "random"    -- generatore mode: 'random' or 'ordered' type sequences
minargs   = 0           -- minimum num. of supported arguments (>= 0)
maxargs   = 60          -- maximum num. of supported arguments (>= minargs)


-- section 'types' -------------------------------------------------------------

types     = "BcCsSiIjJlLpfd" -- supported argument types
rtypes    = nil              -- supported return types (set to nil to use "v"..types)


-- section 'ordered' -----------------------------------------------------------

offset    = 0           -- permutation index offset (default = 0)
step      = 1           -- permutation index increment (default = 1)


-- section 'random' ------------------------------------------------------------
                        
seed      = 40          -- random seed


-- section 'calling convention' (useful on Windows for now) --------------------
                        
api       = ""          -- calling convention ("__stdcall" or "__fastcall")
                        -- for gcc use "__attribute__((__stdcall__))" or "__attribute__((__fastcall__))"
ccprefix  = ""          -- signature prefix ("_s" (stdcall), "_f" (gcc fastcall) or "_F" (microsoft fastcall))
