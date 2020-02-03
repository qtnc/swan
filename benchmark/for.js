var list = [];

var start = new Date().getTime();
for (let i=0; i<2500000; i++)  list.push(i);

var sum = 0;
for (let i of list) sum += i;

start = (new Date().getTime()-start)/1000.0;
console.log(sum)
console.log("elapsed: "+start);
