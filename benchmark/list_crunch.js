let k = [
[10, [2, 5, 10]],
[100, [5, 10, 25]],
[1000, [5, 25, 125]], 
[10000, [4, 40, 400]]
];



let t = new Date().getTime();
let sum = 0;

for (let z=0; z<10; z++) {
for (let [n, w] of k) {
let l = [];
for (let j=0; j<n; j++) l.push(j);
for (let s of w) {
l.sort();
for (let i=0; i<n; i+=s) {
let p = l.slice(i, i+s);
p.reverse();
l.splice(i, s, ...p);
sum += l[l.length -1] + l[0]
}
}
}
}


t = (new Date().getTime()-t)/1000.0;
console.log('Sum: '+sum)
console.log('Elapsed: '+t)