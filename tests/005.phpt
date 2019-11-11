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
Fatal error: Uncaught ilimit\Error\Timeout: the time limit of 1000000 microseconds has been reached in %s:4
Stack trace:
#0 %s(4): sleep(2000)
#1 [internal function]: wait()
#2 %s(10): ilimit('wait', Array, 1000000)
#3 {main}
  thrown in %s on line 4
