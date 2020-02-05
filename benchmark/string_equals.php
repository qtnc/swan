<?php
$start = microtime(true);

$count = 0;
for ($i=0; $i<1000000; $i++) {
  if ("abc" == "abc") $count = $count + 1;
  if ("a slightly longer string" ==
      "a slightly longer string") $count = $count + 1;
  if ("a significantly longer string but still not overwhelmingly long string" ==
      "a significantly longer string but still not overwhelmingly long string") $count = $count + 1;

  if ("" == "abc") $count = $count + 1;
  if ("abc" == "abcd") $count = $count + 1;
  if ("changed one character" == "changed !ne character") $count = $count + 1;
  if ("123" == 123) $count = $count + 1;
  if ("a slightly longer string" ==
      "a slightly longer string!") $count = $count + 1;
  if ("a slightly longer string" ==
      "a slightly longer strinh") $count = $count + 1;
  if ("a significantly longer string but still not overwhelmingly long string" ==
      "another") $count = $count + 1;
}

$start = microtime(true) -$start;
echo $count, "\r\n";
echo "Elapsed: $start";
?>