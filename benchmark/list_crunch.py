import time

k = [
	(10, (2, 5, 10)),
	(100, (5, 10, 25)),
	(1000, (5, 25, 125)), 
	(10000, (4, 40, 400)) 
]



t = time.clock()
sum = 0
for z in range(10):
	for n, w in k:
		l = [x for x in range(n)]
		for s in w:
			l.sort()
			for i in range(0, n, s):
				p = l[i:i+s]
				p.reverse()
				l[i:i+s] = p
				sum+=l[-1]+l[0]


t = time.clock() -t
print('Sum: ' + str(sum))
print('Elapsed: '+str(t))