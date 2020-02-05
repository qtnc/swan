<?php
$n = 35000;
$t = microtime(true);
$sum = 0;

$a = array();
$b = array();
$c = array();
for ($i=0; $i<$n; $i++) {
$a[]=$i;
array_unshift($b, $i);
array_splice($c, $i/2, 0, $i);
$sum += $a[0] + $b[0] + $c[0] + $a[$i ] + $b[$i ] + $c[$i ];
}

$t = microtime(true) -$t;
echo "Sum: $sum\r\nElapsed: $t\r\n"
?>