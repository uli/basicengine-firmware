require "config"
require "math"
require "string"


function randomSignatures()
  package.path = '../common/?.lua;' .. package.path
  require"rand-sig"
end


function orderedSignatures()
  package.path = '../common/?.lua;' .. package.path
  require"ordered-sig"
end


function designedSignatures()
 for line in io.lines(designfile) do
   io.write( line )
   io.write( "\n" )
 end
end


if mode == "random" then
  randomSignatures()
elseif mode == "ordered" then
  orderedSignatures()
elseif mode == "designed" then
  designedSignatures()
else
  error("'mode' must be 'random', 'ordered' or 'designed'")
end

io.flush()

