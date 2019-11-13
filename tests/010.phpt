--TEST--
Check ilimit interrupts constructor
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php
class Foo {
    public function __construct() {
        sleep(10);
    }
}

try {
    \ilimit(function(){
        new Foo();
    }, [], 1000000);
} catch (\ilimit\Error\Timeout $e) {
    echo "OK\n";
}
?>
--EXPECTF--
OK

