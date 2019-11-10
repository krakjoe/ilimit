--TEST--
Check ilimit memory
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php
\ilimit(function(){
    while(true) {
        $array[] = \mt_rand();
    }
}, [], 100000000, 10000);
?>
--EXPECTF--
Fatal error: Uncaught ilimit\Error\Memory: the memory limit of %d bytes has been reached in %s:6
Stack trace:
#0 %s/002.php(6): ilimit(Object(Closure), Array, 100000000, 10000)
#1 {main}
  thrown in %s/002.php on line 6

