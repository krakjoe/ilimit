--TEST--
Check ilimit interrupts foreach over non-array
--SKIPIF--
<?php
if (!extension_loaded('ilimit')) {
	echo 'skip';
}
?>
--FILE--
<?php
try {
    \ilimit\call(function($data){
        foreach ($data as $key => $value) {
            sleep(10);
        }
    }, [new class {
        public $a = "apples";
        public $b = "oranges";
    }], 1000000);
} catch (\ilimit\Error\Timeout $e) {
    echo "OK\n";
}
?>
--EXPECTF--
OK

