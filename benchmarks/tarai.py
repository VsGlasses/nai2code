import sys
import string

def tarai (x, y, z):
	if x <= y:
		return y
	else:
		return tarai(
            tarai(x - 1, y, z),
			tarai(y - 1, z, x),
			tarai(z - 1, x, y))

[x, y, z] = [5,2,0]
print ('tarai(%d, %d, %d) = %d' % (x, y, z, tarai(x, y, z)))