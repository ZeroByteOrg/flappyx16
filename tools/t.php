#!/usr/bin/php -f
<?php

$a = array (65,66);
$b = array_merge($a, array (67,68));
$c = 69;
$d = array (70,71);

print pack("C",$b) . "\n";
