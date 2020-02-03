<?php
ini_set('memory_limit', '1G');
$list = array();
$start = microtime(true);
for ($i=0; $i<2500000; $i++)  $list[]=$i;

$sum = 0;
for ($i=0, $c=count($list); $i<$c; $i++) $sum+=$list[$i];

$start = microtime(true) -$start;
echo $sum, "\r\n";
echo "Elapsed: $start";
?>
