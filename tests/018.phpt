--TEST--
Check timeout with recursive functions
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php

function fib(int $n) : int {
	return ($n === 0 || $n === 1) ? $n : fib($n - 1) + fib($n - 2);
}

try {
    \ilimit\call('fib', [10 ** 6], 10 * 1000);
} catch (\ilimit\Error\Timeout $t) {
    echo "OK";
}

?>
--EXPECT--
OK

