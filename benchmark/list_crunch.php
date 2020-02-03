<?php
ini_set('memory_limit', '1G');
$k = array(
array(10, array(2, 5, 10)),
array(100, array(5, 10, 25)),
array(1000, array(5, 25, 125)), 
array(10000, array(4, 40, 400)) 
);



$t = microtime(true);
$sum = 0;

for ($z=0; $z<10; $z++) {
foreach($k as $nw) {
$n = $nw[0]; $w = $nw[1];
$l = array();
for ($i=0; $i<$n; $i++) $l[]=$i;
foreach($w as $s) {
sort($l);
for ($i=0; $i<$n; $i+=$s) {
$p = array_slice($l, $i, $s);
array_reverse($p);
array_splice($l, $i, $s, $p);
$sum += $l[count($l)-1] + $l[0];
}
}
}
}


$t = microtime(true) -$t;
echo "Sum: $sum\r\n";
echo "Elapsed: $t";