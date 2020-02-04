var n = 35000
var t = System.clock
var sum = 0

var a = []
var b = []
var c = []

for (i in 0...n) {
a.add(i)
b.insert(0, i)
c.insert((i/2).floor, i)
sum = sum + a[0] + b[0] + c[0] + a[-1] + b[-1] + c[-1]
}

System.print("Sum: %(sum)")
System.print("elapsed: %(System.clock - t)")