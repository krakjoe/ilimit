--TEST--
Check ilimit catch cpu
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
        sleep(2);
    }, [], 1000000);
} catch (\ilimit\Error\CPU $ex) {
    echo "OK\n";
}
?>
--EXPECT--
OK
