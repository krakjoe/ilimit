--TEST--
Check ilimit catch memory
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php
try {
    \ilimit\call(function(){
        while(true) {
            $array[] = [4,2];
        }
    }, [], 100000000, 10000);
} catch (\ilimit\Error\Memory $ex) {
    echo "OK\n";
}
?>
--EXPECT--
OK
