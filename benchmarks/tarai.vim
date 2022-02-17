vim9script

def Tarai(x: number, y: number, z: number): number
  count += 1
  if x <= y
    return y
  endif
  return Tarai(
	Tarai(x - 1, y, z),
	Tarai(y - 1, z, x),
	Tarai(z - 1, x, y)
  )
enddef

var count = 0
var start = reltime()
echo Tarai(12, 6, 0)
echo count
echo reltimestr(reltime(start))