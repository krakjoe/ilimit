--TEST--
Check ilimit runtime invalid memory
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php
try {
    \ilimit\call(function(){}, [], 1000000, -1);
} catch (\ilimit\Error\Runtime $e) {
    var_dump($e->getMessage());
}
?>
--EXPECT--
string(27) "memory must be non negative"

