function fib (n) {
if (n<2) return n;
    return fib(n -1) + fib(n -2)
  }


var start = new Date().getTime()
for (let i=0; i<5; i++) {
console.log(fib(28));
}
start = (new Date().getTime() -start) /1000.0;
console.log("Elapsed: " +start);
