local n = 35000
local t = os.clock()
local sum = 0
local insert, ceil = table.insert, math.ceil

local a, b, c = {}, {}, {}
for i = 1, n  do
insert(a, i)
insert(b, 1, i)
insert(c, ceil(i/2), i)
sum = sum + a[1] + b[1] + c[1] + a[i] + b[i] + c[i]
end

t = os.clock()-t
io.write('Sum: ' ..sum .. '\n')
io.write(string.format("elapsed: %.8f\n", t))