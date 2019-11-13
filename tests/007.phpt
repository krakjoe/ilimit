--TEST--
Check nested ilimit calls
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php
\ilimit\call(function(){
    \ilimit\call(function(){
        sleep(10);
    }, [], 500000);
}, [], 1000000);
?>
--EXPECTF--
Fatal error: Uncaught ilimit\Error\Timeout: the time limit of 500000 microseconds has been reached in %s:4
Stack trace:
#0 %s(4): sleep(10)
#1 [internal function]: {closure}()
#2 %s(5): ilimit\call(Object(Closure), Array, 500000)
#3 [internal function]: {closure}()
#4 %s(6): ilimit\call(Object(Closure), Array, 1000000)
#5 {main}
  thrown in %s on line 4
