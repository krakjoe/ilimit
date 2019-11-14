--TEST--
Check ilimit runtime invalid timeout
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php
try {
    \ilimit\call(function(){}, [], 0);
} catch (\ilimit\Error\Runtime $e) {
    var_dump($e->getMessage());
}
?>
--EXPECT--
string(24) "timeout must be positive"


