--TEST--
Check ilimit runtime precondition memory
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php
$usage = \memory_get_usage();

\ini_set("memory_limit", $usage);

try {
    \ilimit\call(function(){}, [], 1000000, $usage - 10);
} catch (\ilimit\Error\Memory $e) {
    var_dump($e->getMessage());
}
?>
--EXPECTF--
string(%d) "memory limit of %d bytes would be exceeded"


