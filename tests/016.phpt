--TEST--
Check ilimit rope interrupt
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
    \ilimit\call(function(){
        $function = function(){
            sleep(10);
        };

        $variable = "variable";
        $rope = "rope";
        $string = "string {$variable} {$rope} {$function()})}";
    }, [], 1000000);
} catch (\ilimit\Error\Timeout $e) {
    echo "OK";
}
?>
--EXPECT--
OK


