let n = 35000;
let t = new Date().getTime();
let sum = 0;

let a = [], b = [], c = []
for (let i=0; i<n; i++) {
a.push(i);
b.unshift(i);
c.splice(i/2, 0, i);
sum += a[0] + b[0] + c[0] + a[i] + b[i] + c[i]
}

t = (new Date().getTime() -t)/1000.0;
console.log('Sum: ' +sum);
console.log('Elapsed: '+t);