<?php
$start = microtime(true);

$abc1 = "abc";
$abc2 = "abc";
$l1 = "a slightly longer string";
$l2 = "a slightly longer string";
$ll1 = "a significantly longer string but still not overwhelmingly long string";
$ll2 = "a significantly longer string but still not overwhelmingly long string";
$c1 = "changed one character";
$c2 = "changed !ne character";
$numstr = "123";
$al1 = "a slightly longer string";
$al2 = "a slightly longer string!";


$count = 0;
for ($i=0; $i<1000000; $i++) {
  if ($abc1 == $abc2) $count = $count + 1;
  if ($l1 ==      $l2) $count = $count + 1;
  if ($ll1 == $ll2) $count = $count + 1;

  if ("" == $abc1) $count = $count + 1;
  if ($abc1 == "abcd") $count = $count + 1;
  if ($c1 == $c2) $count = $count + 1;
  if ($numstr == 123) $count = $count + 1;
  if ($al1 ==      $al2) $count = $count + 1;
  if ($al1 ==      "a slightly longer strinh") $count = $count + 1;
  if ($ll1 ==       "another") $count = $count + 1;
}

$start = microtime(true) -$start;
echo "$count\r\n";
echo "Elapsed: $start";
?>