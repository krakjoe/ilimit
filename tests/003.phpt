--TEST--
Check ilimit catch Timeout
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php
try {
    \ilimit(function(){
        sleep(5);
    }, [], 1000000);
} catch (\ilimit\Error\Timeout $ex) {
    echo "OK\n";
}
?>
--EXPECT--
OK
