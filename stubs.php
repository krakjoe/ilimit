<?php

declare(strict_types=1);

namespace ilimit {
    /**
     * Call a callback while imposing limits on the CPU time and memory that
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
    function ilimit(
        callable $callable,
        array $arguments,
        int $timeout,
        int $maxMemory = 0,
        int $checkInterval = 0
    ) {}

    /**
     * Thrown when the invocation exceeds the allowed limits.
     */
    class Error extends \Exception
    {
    }
}

namespace ilimit\Error {

    use ilimit\Error;

    /**
     * Thrown when the system lacks necessary resources to make the call.
     */
    class System extends Error
    {
    }

    /**
     * Thrown when the invocation exceeds the allowed time.
     */
    class Timeout extends Error
    {
    }

    /**
     * Thrown when the invocation exceeds the allowed memory.
     */
    class Memory extends Error
    {
    }
}

