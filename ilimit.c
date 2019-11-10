/*
  +----------------------------------------------------------------------+
  | ilimit                                                               |
  +----------------------------------------------------------------------+
  | Copyright (c) Joe Watkins 2019                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: krakjoe                                                      |
  +----------------------------------------------------------------------+
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_ilimit.h"

#include "zend_exceptions.h"
#include "zend_closures.h"

#ifdef ZTS
# error "Cannot support thread safe builds"
#else
# include <pthread.h>
#endif

zend_class_entry *php_ilimit_sys_ex;
zend_class_entry *php_ilimit_cpu_ex;
zend_class_entry *php_ilimit_memory_ex;

typedef struct _php_ilimit_call_t {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    zend_ulong      state;

    struct _php_ilimit_call_threads {
        pthread_t cpu;
        pthread_t memory;
    } threads;

    struct _php_ilimit_call_zend {
        zend_fcall_info       info;
        zend_fcall_info_cache cache;
        zend_execute_data    *entry;
        zend_execute_data    *frame;
    } zend;

    struct _php_ilimit_call_limits {
        zend_ulong cpu;
        struct _memory {
            zend_ulong max;
            zend_ulong interval;
        } memory;
    } limits;

} php_ilimit_call_t;

#define PHP_ILIMIT_RUNNING  0x00000001
#define PHP_ILIMIT_FINISHED 0x00000010
#define PHP_ILIMIT_TIMEOUT  0x00000100
#define PHP_ILIMIT_MEMORY   0x00001000

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(ilimit)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "ilimit", "Error\\System", NULL);

    php_ilimit_sys_ex =
        zend_register_internal_class_ex(&ce, zend_ce_exception);
    php_ilimit_sys_ex->ce_flags |= ZEND_ACC_FINAL;

    INIT_NS_CLASS_ENTRY(ce, "ilimit", "Error\\CPU", NULL);

    php_ilimit_cpu_ex =
        zend_register_internal_class_ex(&ce, zend_ce_exception);
    php_ilimit_cpu_ex->ce_flags |= ZEND_ACC_FINAL;

    INIT_NS_CLASS_ENTRY(ce, "ilimit", "Error\\Memory", NULL);

    php_ilimit_memory_ex =
        zend_register_internal_class_ex(&ce, zend_ce_exception);
    php_ilimit_memory_ex->ce_flags |= ZEND_ACC_FINAL;

    return SUCCESS;
} /* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(ilimit)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "ilimit support", "enabled");
    php_info_print_table_end();
}
/* }}} */

static zend_always_inline void php_ilimit_clock(struct timespec *clock, zend_ulong ms) { /* {{{ */
    struct timeval time;
    
    if (ms == 0) {
        return;
    }
    
    gettimeofday(&time, NULL);

    time.tv_sec += (ms / 1000000UL);
    time.tv_sec += (time.tv_usec + (ms % 1000000UL)) / 1000000UL;
    time.tv_usec = (time.tv_usec + (ms % 1000000UL)) % 1000000UL;

    clock->tv_sec = time.tv_sec;
    clock->tv_nsec = time.tv_usec * 1000;
} /* }}} */

static void* __php_ilimit_call_thread(void *arg) { /* {{{ */
    php_ilimit_call_t *call =
        (php_ilimit_call_t*) arg;
    int __unused;

    pthread_mutex_lock(&call->mutex);
    call->state |= PHP_ILIMIT_RUNNING;
    pthread_cond_broadcast(&call->cond);
    pthread_mutex_unlock(&call->mutex);

    pthread_setcanceltype(
        PTHREAD_CANCEL_ASYNCHRONOUS, &__unused);
    zend_call_function(
            &call->zend.info, &call->zend.cache);

    pthread_mutex_lock(&call->mutex);
    call->state &= ~PHP_ILIMIT_RUNNING;
    call->state |= PHP_ILIMIT_FINISHED;
    pthread_cond_broadcast(&call->cond);
    pthread_mutex_unlock(&call->mutex);

    pthread_exit(NULL);
} /* }}} */

static void* __php_ilimit_memory_thread(void *arg) { /* {{{ */
    php_ilimit_call_t *call =
        (php_ilimit_call_t*) arg;
    struct timespec clock;

    pthread_mutex_lock(&call->mutex);

    while (!(call->state & PHP_ILIMIT_RUNNING)) {
        pthread_cond_wait(&call->cond, &call->mutex);
    }

    pthread_mutex_unlock(&call->mutex);

    /* the call is running ... start checking memory */

    pthread_mutex_lock(&call->mutex);

    while (!(call->state & (PHP_ILIMIT_FINISHED|PHP_ILIMIT_TIMEOUT))) {
        php_ilimit_clock(&clock, call->limits.memory.interval);

        switch (pthread_cond_timedwait(&call->cond, &call->mutex, &clock)) {
            case SUCCESS:
                /* do nothing, signalled */
            break;

            case ETIMEDOUT:
                if (zend_memory_usage(0) > call->limits.memory.max) {
                    call->zend.frame = EG(current_execute_data);

                    call->state |=
                        PHP_ILIMIT_MEMORY|PHP_ILIMIT_FINISHED;

                    pthread_cond_broadcast(&call->cond);
                    pthread_mutex_unlock(&call->mutex);

                    pthread_cancel(call->threads.cpu);

                    goto php_ilimit_memory_finish;
                }
            break;
        }
    }

    pthread_mutex_unlock(&call->mutex);

php_ilimit_memory_finish:
    pthread_exit(NULL);
} /* }}} */

static zend_always_inline int php_ilimit_memory(php_ilimit_call_t *call) { /* {{{ */
    if (call->limits.memory.max == 0) {
        return SUCCESS;
    }

    call->limits.memory.max += zend_memory_usage(0);

    if (call->limits.memory.max > PG(memory_limit)) {
        return FAILURE;
    }

    if (!call->limits.memory.interval) {
        call->limits.memory.interval = 100;
    }

    return SUCCESS;
} /* }}} */

static zend_always_inline void php_ilimit_call_init(php_ilimit_call_t *call, zend_execute_data *entry) { /* {{{ */
    memset(call, 0, sizeof(php_ilimit_call_t));

    pthread_mutex_init(&call->mutex, NULL);
    pthread_cond_init(&call->cond, NULL);

    call->zend.entry = entry;
} /* }}} */

static zend_always_inline void php_ilimit_call(php_ilimit_call_t *call) { /* {{{ */
    struct timespec clock;

    pthread_mutex_lock(&call->mutex);

    if (call->limits.memory.max) {
        if (php_ilimit_memory(call) != SUCCESS) {
            zend_throw_exception_ex(php_ilimit_memory_ex, 0,
                "memory limit of %" PRIu64 " bytes would be exceeded",
                PG(memory_limit));
            call->state |= PHP_ILIMIT_FINISHED;
            pthread_mutex_unlock(&call->mutex);
            return;
        }

        if (pthread_create(&call->threads.memory, NULL, __php_ilimit_memory_thread, call) != SUCCESS) {
            zend_throw_exception_ex(php_ilimit_sys_ex, 0,
                "cannot create memory management thread");
            call->state |= PHP_ILIMIT_FINISHED;
            pthread_mutex_unlock(&call->mutex);
            return;
        }
    }

    php_ilimit_clock(&clock, call->limits.cpu);

    if (pthread_create(&call->threads.cpu, NULL, __php_ilimit_call_thread, call) != SUCCESS) {
        zend_throw_exception_ex(php_ilimit_sys_ex, 0,
            "cannot create cpu management thread");
        call->state |= PHP_ILIMIT_FINISHED;
        pthread_cond_broadcast(&call->cond);
        pthread_mutex_unlock(&call->mutex);

        if (call->limits.memory.max) {
            pthread_join(call->threads.memory, NULL);
        }
        return;
    }

    while (!(call->state & PHP_ILIMIT_FINISHED)) {
        switch (pthread_cond_timedwait(&call->cond, &call->mutex, &clock)) {
            case SUCCESS:
                /* do nothing, signalled */
            break;

            case ETIMEDOUT: {
                call->zend.frame =
                    EG(current_execute_data);

                call->state |= PHP_ILIMIT_TIMEOUT;

                pthread_cond_broadcast(&call->cond);
                pthread_mutex_unlock(&call->mutex);
                pthread_cancel(call->threads.cpu);
            }

            goto php_ilimit_call_finish;
        }
    }

    pthread_mutex_unlock(&call->mutex);

php_ilimit_call_finish:
    pthread_join(call->threads.cpu, NULL);

    if (call->limits.memory.max) {
        pthread_join(call->threads.memory, NULL);
    }
} /* }}} */

static zend_always_inline void php_ilimit_call_cleanup(php_ilimit_call_t *call) { /* {{{ */
    zend_execute_data *execute_data = call->zend.frame,
                      *execute_entry = call->zend.entry;

    while (execute_data && execute_data != execute_entry) {
        zend_execute_data *prev;
        zval *var = EX_VAR_NUM(0),
             *end;

        if (EX(func)->type == ZEND_USER_FUNCTION) {
            end = var +
                (EX(func)->op_array.last_var + EX(func)->op_array.T);
        } else {
            end = var + EX(func)->common.num_args;
        }

        while (var < end) {
            if (!Z_ISUNDEF_P(var) && Z_REFCOUNTED_P(var)) {
                zval_ptr_dtor(var);
            }
            var++;
        }

        if (EX(func)->common.fn_flags & ZEND_ACC_CLOSURE) {
            OBJ_RELEASE(ZEND_CLOSURE_OBJECT(EX(func)));
        }

        prev = EX(prev_execute_data);

        if (prev != execute_entry) {
            zend_vm_stack_free_call_frame(execute_data);
        }

        execute_data = prev;
    }
} /* }}} */

static zend_always_inline void php_ilimit_call_destroy(php_ilimit_call_t *call) { /* {{{ */
    if (call->state & (PHP_ILIMIT_TIMEOUT|PHP_ILIMIT_MEMORY)) {
        php_ilimit_call_cleanup(call);
    }

    EG(current_execute_data) = call->zend.entry;

    if (call->state & PHP_ILIMIT_TIMEOUT) {
        zend_throw_exception_ex(php_ilimit_cpu_ex, 0,
            "the cpu time limit of %" PRIu64 " microseconds has been reached",
            call->limits.cpu);
    }

    if (call->state & PHP_ILIMIT_MEMORY) {
        zend_throw_exception_ex(php_ilimit_memory_ex, 0,
            "the memory limit of %" PRIu64 " bytes has been reached",
            call->limits.memory.max);
    }

    pthread_mutex_destroy(&call->mutex);
    pthread_cond_destroy(&call->cond);
} /* }}} */

/* {{{ arg info */
ZEND_BEGIN_ARG_INFO_EX(php_ilimit_arginfo, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, callable, IS_CALLABLE, 0)
    ZEND_ARG_TYPE_INFO(0, arguments, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, cpu, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, memory, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, interval, IS_LONG, 0)
ZEND_END_ARG_INFO() /* }}} */

/* {{{ proto mixed \ilimit(callable $function [, array $args, int $cpuMS, int $memoryBytes, int $intervalMs = 100]) */
PHP_FUNCTION(ilimit)
{
    php_ilimit_call_t call;
    zval *args = NULL;

    php_ilimit_call_init(&call, execute_data);

    ZEND_PARSE_PARAMETERS_START(1, 5)
        Z_PARAM_FUNC(call.zend.info, call.zend.cache)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(args)
        Z_PARAM_LONG(call.limits.cpu)
        Z_PARAM_LONG(call.limits.memory.max)
        Z_PARAM_LONG(call.limits.memory.interval)
    ZEND_PARSE_PARAMETERS_END();

    call.zend.info.retval = return_value;

    if (args) {
        zend_fcall_info_args(&call.zend.info, args);
    }

    php_ilimit_call(&call);

    if (args) {
        zend_fcall_info_args_clear(&call.zend.info, 1);
    }

    php_ilimit_call_destroy(&call);
} /* }}} */

/* {{{ php_ilimit_functions[]
 */
static const zend_function_entry php_ilimit_functions[] = {
    PHP_FE(ilimit, php_ilimit_arginfo)
    PHP_FE_END
};
/* }}} */

/* {{{ ilimit_module_entry
 */
zend_module_entry ilimit_module_entry = {
    STANDARD_MODULE_HEADER,
    "ilimit",
    php_ilimit_functions,
    PHP_MINIT(ilimit),
    NULL,
    NULL,
    NULL,
    PHP_MINFO(ilimit),
    PHP_ILIMIT_VERSION,
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_ILIMIT
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(ilimit)
#endif
