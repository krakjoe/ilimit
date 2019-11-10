--TEST--
Check ilimit Timeout
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php
\ilimit(function(){
    sleep(5);
}, [], 1000000);
?>
--EXPECTF--
Fatal error: Uncaught ilimit\Error\Timeout: the time limit of 1000000 microseconds has been reached in %s/001.php:4
Stack trace:
#0 %s/001.php(4): ilimit(Object(Closure), Array, 1000000)
#1 {main}
  thrown in %s/001.php on line 4
