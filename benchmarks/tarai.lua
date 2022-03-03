local function tarai(x, y, z)
	if x <= y then return y end
	return tarai(
		tarai(x-1, y, z),
		tarai(y-1, z, x),
		tarai(z-1, x, y)
	)
end

local write, format = io.write, string.format
write(format("%d\n",tarai(12,6,0)))
