def tarai(x, y, z)
  if x <= y
    y
  else
    tarai(
      tarai(x - 1, y, z),
      tarai(y - 1, z, x),
      tarai(z - 1, x, y)
    )
  end
end

print tarai(12, 6, 0)
