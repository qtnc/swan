var start = System.clock

var abc1 = "abc" 
var abc2 = "abc" 
var l1 = "a slightly longer string" 
var l2 = "a slightly longer string" 
var ll1 = "a significantly longer string but still not overwhelmingly long string" 
var ll2 = "a significantly longer string but still not overwhelmingly long string" 
var c1 = "changed one character" 
var c2 = "changed !ne character" 
var numstr = "123" 
var al1 = "a slightly longer string" 
var al2 = "a slightly longer string!" 


var count = 0
for (i in 1..1000000) {
  if (abc1 == abc2) count = count + 1
  if (l1 ==      l2) count = count + 1
  if (ll1 == ll2) count = count + 1

  if ("" == abc1) count = count + 1
  if (abc1 == "abcd") count = count + 1
  if (c1 == c2) count = count + 1
  if (numstr == 123) count = count + 1
  if (al1 ==      al2) count = count + 1
  if (al1 ==      "a slightly longer strinh") count = count + 1
  if (ll1 ==       "another") count = count + 1
}

System.print(count)
System.print("elapsed: %(System.clock - start)")
