--TEST--
Check ilimit restores silence
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
        @sleep(10);
    }, [], 1000000);
} catch (\ilimit\Error\Timeout $e) {}

fopen('.non_existent_path', 'r');
?>
--EXPECTF--
Warning: fopen(.non_existent_path): failed to open stream: No such file or directory in %s on line 8

