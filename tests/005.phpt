--TEST--
Check finally blocks execution
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php
function wait() {
    try {
        sleep(2000);
    } finally {
        echo 'finalized';
    }
}

\ilimit('wait', [], 1000000);
?>
--EXPECTF--
finalized
Fatal error: Uncaught ilimit\Error\CPU: the cpu time limit of 1000000 microseconds has been reached in %s/005.php:10
Stack trace:
#0 %s/005.php(10): ilimit(Object(Closure), Array, 1000000)
#1 {main}
  thrown in %s/005.php on line 10