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
        $array[] = [4,2];
    }
}, [], 100000000, 10000);
?>
--EXPECTF--
Fatal error: Uncaught ilimit\Error\Memory: the memory limit of %d bytes has been reached in %s:4
Stack trace:
#0 [internal function]: {closure}()
#1 %s(6): ilimit(Object(Closure), Array, 100000000, 10000)
#2 {main}
  thrown in %s on line 4

