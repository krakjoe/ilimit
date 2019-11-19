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

try {
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
} catch (\ilimit\Error\Timeout $t) {
    echo "OK";
}

?>
--EXPECT--
OK
