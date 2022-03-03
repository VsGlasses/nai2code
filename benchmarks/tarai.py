def tarai (x,y,z):
    return y if x <= y else tarai (
        tarai (x-1,y,z),
        tarai (y-1,z,x),
        tarai (z-1,x,y)
    )

print (tarai (12,6,0))