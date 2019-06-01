local start = os.clock()

local count = 0
for i = 1, 1000000 do
  if ("abc" == "abc") then count = count + 1 end
  if ("a slightly longer string" ==
      "a slightly longer string") then count = count + 1 end
  if ("a significantly longer string but still not overwhelmingly long string" ==
      "a significantly longer string but still not overwhelmingly long string") then count = count + 1 end

  if ("" == "abc") then count = count + 1 end
  if ("abc" == "abcd") then count = count + 1 end
  if ("changed one character" == "changed !ne character") then count = count + 1 end
  if ("123" == 123) then count = count + 1 end
  if ("a slightly longer string" ==
      "a slightly longer string!") then count = count + 1 end
  if ("a slightly longer string" ==
      "a slightly longer strinh") then count = count + 1 end
  if ("a significantly longer string but still not overwhelmingly long string" ==
      "another") then count = count + 1 end
end

print(count)
print("elapsed: "..(os.clock() - start))
