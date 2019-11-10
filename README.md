ilimit
======

`ilimit` provides a method to execute a call while imposing limits on the CPU time and memory that the call may consume.

Requirements
============

  * PHP 7.1+
  * NTS
  * pthread.h

API
===

```php
/**
* @param callable the invocation to make
* @param array    arguments to pass to callable
* @param integer  the maximum time allowed for invocation of callable in milliseconds
* @param integer  the maximum amount of memory the invocation may consume
* @param integer  the millisecond interval between memory check
*
* @throws \ilimit\Error\System  should the system lack necessary resources to make the call
* @throws \ilimit\Error\CPU     should the invocation exceed allowed time
* @throws \ilimit\Error\Memory  should the invocation exceed allowed memory

* @return return value of invocation
*/
function ilimit(callable $callable, array $args = [], integer $timeoutMs, integer $memoryBytes = 0, integer $memoryCheckIntervalMs = 0) : mixed;
```


