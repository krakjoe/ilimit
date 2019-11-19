--TEST--
Check timeout with foreach
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php

\ilimit\call(function () {
	$generator = (function () : \Generator {
		$i = 0;
		while (true) {
			yield $i++;
		}
	})();

	foreach ($generator as $key => $value) {
		continue;
	}
}, [], 9000);

?>
--EXPECTF--
Fatal error: Uncaught ilimit\Error\Timeout: the time limit of 9000 microseconds has been reached in %s:11
Stack trace:
#0 [internal function]: {closure}()
#1 %s(14): ilimit\call(Object(Closure), Array, 9000)
#2 {main}
  thrown in %s on line 11
