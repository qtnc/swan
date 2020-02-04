import time

n = 35000
t = time.clock()
sum = 0

a, b, c = [], [], []
for i in range(n):
	a.append(i)
	b.insert(0, i)
	c.insert(i//2, i)
	sum += a[0] + b[0] + c[0] + a[-1] + b[-1] + c[-1]


t = time.clock()-t
print('Sum: '+str(sum))
print('Elapsed: '+str(t))