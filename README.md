ilimit
======

`ilimit` provides a method to execute a call while imposing limits on the time and memory that the call may consume.

Requirements
============

  * PHP 7.1+
  * NTS
  * pthread.h
  
Stubs
=====

This repository includes PHP files with method headers for IDE integration and 
static analysis support.

To install, run the following command:
```
composer require krakjoe/ilimit
```

API
===

```php
/**
 * Call a callback while imposing limits on the time and memory that
 * the call may consume.
 *
 * @param callable $callable      The invocation to make.
 * @param array    $arguments     The list of arguments.
 * @param int      $timeout       The maximum execution time, in microseconds.
 * @param int      $maxMemory     The maximum amount of memory, in bytes.
 *                                If set to zero, no limit is imposed.
 * @param int      $checkInterval The interval between memory checks,
 *                                in microseconds. If set to zero or less,
 *                                a default interval of 100 microseconds is used.
 *
 * @return mixed Returns the return value of the callback.
 *
 * @throws Error\System  If the system lacks necessary resources to make the call.
 * @throws Error\Timeout If the invocation exceeds the allowed time.
 * @throws Error\Memory  If the invocation exceeds the allowed memory.
 */
function ilimit(callable $callable, array $arguments, int $timeout, int $maxMemory = 0, int $checkInterval = 0)
```


