--TEST--
Check timeout within while loops
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
Fatal error: Uncaught ilimit\Error\Timeout: the time limit of 1000000 microseconds has been reached in %s:3
Stack trace:
#0 [internal function]: {closure}()
#1 %s(4): ilimit(Object(Closure), Array, 1000000)
#2 {main}
  thrown in %s on line 3
