<?php
function fib ($n) {
if ($n < 2) return $n;
    return fib($n -1) + fib($n -2);
  }


$start = microtime(true);
for ($i=0; $i<5; $i++) {
echo fib(28), "\r\n";
}
$start = microtime(true) -$start;
print("Elapsed: $start");
