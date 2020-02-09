local start = os.clock()

local abc1 = "abc";
local  abc2 = "abc";
local  l1 = "a slightly longer string";
local  l2 = "a slightly longer string";
local  ll1 = "a significantly longer string but still not overwhelmingly long string";
local  ll2 = "a significantly longer string but still not overwhelmingly long string";
local  c1 = "changed one character";
local  c2 = "changed !ne character";
local  numstr = "123";
local  al1 = "a slightly longer string";
local  al2 = "a slightly longer string!";


local  count = 0
for i = 1, 1000000 do
  if (abc1 == abc2) then count = count + 1 end
  if (l1 ==      l2) then count = count + 1 end
  if (ll1 == ll2) then count = count + 1 end

  if ("" == abc1) then count = count + 1 end
  if (abc1 == "abcd") then count = count + 1 end
  if (c1 == c2) then count = count + 1 end
  if (numstr == 123) then count = count + 1 end
  if (al1 ==      al2) then count = count + 1 end
  if (al1 ==      "a slightly longer strinh") then count = count + 1 end
  if (ll1 ==       "another") then count = count + 1 end
end

start = os.clock()-start
print(count)
print('Elapsed: ' .. start)
