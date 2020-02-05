<?php
$start = microtime(true);

$n = 1000000;
$map = array();

for ($i=0; $i<=$n; $i++) {
$map[$i]=$i;
}

$sum = 0;
for ($i=0; $i<=$n; $i++) {
$sum+=$map[$i];
}

echo $sum, "\r\n";

for ($i=0; $i<=$n; $i++) {
unset($map[$i]);
}

$start = microtime(true) -$start;
echo "Elapsed: $start";
?>