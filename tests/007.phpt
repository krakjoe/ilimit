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
\ilimit(function(){
    \ilimit(function(){
        sleep(10);
    }, [], 500000);
}, [], 1000000);
?>
--EXPECTF--
Fatal error: Uncaught ilimit\Error\CPU: the cpu time limit of 500000 microseconds has been reached in %s/007.php:5
Stack trace:
#0 %s/007.php(5): ilimit(Object(Closure), Array, 500000)
#1 [internal function]: {closure}()
#2 %s/007.php(6): ilimit(Object(Closure), Array, 1000000)
#3 {main}
  thrown in %s/007.php on line 5
