var start = new Date().getTime();

var n = 1000000
var map = new Map();

for (var i=0; i<=n; i++) {
map.set(i, i);
}

var sum = 0
for (var i=0; i<=n; i++) {
  sum += map.get(i);
}

console.log(sum);

for (var i=0; i<=n; i++) {
map.delete(i);
}

start = (new Date().getTime() -start)/1000.0;
console.log("Elapsed: " +start);
