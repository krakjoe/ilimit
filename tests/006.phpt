--TEST--
Check ilimit cpu within while loops
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php
\ilimit(function(){
    while(true);
}, [], 1000000);
?>
--EXPECTF--
Fatal error: Uncaught ilimit\Error\CPU: the cpu time limit of 1000000 microseconds has been reached in %s/006.php:4
Stack trace:
#0 %s/006.php(4): ilimit(Object(Closure), Array, 1000000)
#1 {main}
  thrown in %s/006.php on line 4
