def tarai (x,y,z):
    global count
    count += 1
    return y if x <= y else tarai (
        tarai (x-1,y,z),
        tarai (y-1,z,x),
        tarai (z-1,x,y)
    )

count = 0
print (tarai (12,6,0),"(count:",count,")")