--TEST--
Check ilimit free extra args
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php
try {
    \ilimit\call(function($one){
        sleep(10);
    }, ["one", "two", "three"], 1000000);
} catch (\ilimit\Error\Runtime $e) {
    echo "OK\n";
}
?>
--EXPECTF--
Fatal error: Uncaught ilimit\Error\Timeout: the time limit of 1000000 microseconds has been reached in %s:4
Stack trace:
#0 %s(4): sleep(10)
#1 [internal function]: {closure}('one', 'two', 'three')
#2 %s(5): ilimit\call(Object(Closure), Array, 1000000)
#3 {main}
  thrown in %s on line 4


