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
#ifndef HAVE_PHP_ILIMIT_C
#define HAVE_PHP_ILIMIT_C

#include <php.h>

#include "ilimit.h"

zend_class_entry *php_ilimit_ex;
zend_class_entry *php_ilimit_runtime_ex;
zend_class_entry *php_ilimit_sys_ex;
zend_class_entry *php_ilimit_cpu_ex;
zend_class_entry *php_ilimit_memory_ex;

__thread php_ilimit_call_t *__context;

void (*zend_interrupt_callback)(zend_execute_data *);

static void php_ilimit_interrupt(zend_execute_data *execute_data) { /* {{{ */
    if (!__context) {
        if (zend_interrupt_callback) {
            zend_interrupt_callback(execute_data);
        }
        return;
    }

    pthread_mutex_lock(&__context->mutex);

    __context->state |= PHP_ILIMIT_INTERRUPTED;

    pthread_cond_broadcast(&__context->cond);
    pthread_mutex_unlock(&__context->mutex);

    pthread_exit(NULL);
} /* }}} */

void php_ilimit_startup(void) { /* {{{ */
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "ilimit", "Error", NULL);

    php_ilimit_ex =
        zend_register_internal_class_ex(&ce, zend_ce_exception);

    INIT_NS_CLASS_ENTRY(ce, "ilimit", "Error\\Runtime", NULL);

    php_ilimit_runtime_ex =
        zend_register_internal_class_ex(&ce, php_ilimit_ex);
    php_ilimit_runtime_ex->ce_flags |= ZEND_ACC_FINAL;

    INIT_NS_CLASS_ENTRY(ce, "ilimit", "Error\\System", NULL);

    php_ilimit_sys_ex =
        zend_register_internal_class_ex(&ce, php_ilimit_ex);
    php_ilimit_sys_ex->ce_flags |= ZEND_ACC_FINAL;

    INIT_NS_CLASS_ENTRY(ce, "ilimit", "Error\\Timeout", NULL);

    php_ilimit_cpu_ex =
        zend_register_internal_class_ex(&ce, php_ilimit_ex);
    php_ilimit_cpu_ex->ce_flags |= ZEND_ACC_FINAL;

    INIT_NS_CLASS_ENTRY(ce, "ilimit", "Error\\Memory", NULL);

    php_ilimit_memory_ex =
        zend_register_internal_class_ex(&ce, php_ilimit_ex);
    php_ilimit_memory_ex->ce_flags |= ZEND_ACC_FINAL;

    zend_interrupt_callback = zend_interrupt_function;
    zend_interrupt_function = php_ilimit_interrupt;
} /* }}} */

static zend_always_inline void php_ilimit_clock(struct timespec *clock, zend_long ms) { /* {{{ */
    struct timeval time;

    gettimeofday(&time, NULL);

    time.tv_sec += (ms / 1000000L);
    time.tv_sec += (time.tv_usec + (ms % 1000000L)) / 1000000L;
    time.tv_usec = (time.tv_usec + (ms % 1000000L)) % 1000000L;

    clock->tv_sec = time.tv_sec;
    clock->tv_nsec = time.tv_usec * 1000;
} /* }}} */

static void php_ilimit_call_cancel(php_ilimit_call_t *call) {
    zend_bool cancelled = 0;
    zend_long max = 10000, tick = 0;

    pthread_mutex_lock(&call->mutex);

    EG(vm_interrupt) = 1;

    while (!(call->state & PHP_ILIMIT_INTERRUPTED)) {
        struct timespec clock;

        php_ilimit_clock(&clock, 100);

        switch (pthread_cond_timedwait(&call->cond, &call->mutex, &clock)) {
            case ETIMEDOUT:
                if (!cancelled) {
                    pthread_cancel(
                        call->threads.cpu);
                    cancelled = 1;
                }

                if (tick++ > max) {
                    goto __php_ilimit_call_cancel_bail;
                }
            break;

            case SUCCESS:
                /* do nothing, signalled */
                break;
        }
    }

__php_ilimit_call_cancel_bail:
    pthread_mutex_unlock(&call->mutex);
}

static void __php_ilimit_call_thread_cancel(php_ilimit_call_t *call) {
    pthread_mutex_lock(&call->mutex);

    call->state |= PHP_ILIMIT_INTERRUPTED;

    pthread_cond_broadcast(&call->cond);
    pthread_mutex_unlock(&call->mutex);
}

static void* __php_ilimit_call_thread(void *arg) { /* {{{ */
    php_ilimit_call_t *call = __context =
        (php_ilimit_call_t*) arg;

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    pthread_mutex_lock(&call->mutex);
    call->state |= PHP_ILIMIT_RUNNING;
    pthread_cond_broadcast(&call->cond);
    pthread_mutex_unlock(&call->mutex);

    pthread_cleanup_push(
        (void (*)(void*))__php_ilimit_call_thread_cancel, call);

    zend_call_function(
            &call->zend.info, &call->zend.cache);

    pthread_cleanup_pop(0);

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

    while (!(call->state &
        (PHP_ILIMIT_RUNNING|PHP_ILIMIT_FINISHED|PHP_ILIMIT_TIMEOUT))) {
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

                    php_ilimit_call_cancel(call);

                    goto __php_ilimit_memory_finish;
                }
            break;
        }
    }

    pthread_mutex_unlock(&call->mutex);

__php_ilimit_memory_finish:
    pthread_exit(NULL);
} /* }}} */

static zend_always_inline int php_ilimit_memory(php_ilimit_call_t *call) { /* {{{ */
    call->limits.memory.max += zend_memory_usage(0);

    if (call->limits.memory.max > PG(memory_limit)) {
        return FAILURE;
    }

    if (call->limits.memory.interval <= 0) {
        call->limits.memory.interval = 100;
    }

    return SUCCESS;
} /* }}} */

void php_ilimit_call_init(php_ilimit_call_t *call, zend_execute_data *entry) { /* {{{ */
    memset(call, 0, sizeof(php_ilimit_call_t));

    pthread_mutex_init(&call->mutex, NULL);
    pthread_cond_init(&call->cond, NULL);

    call->zend.entry = entry;
} /* }}} */

static zend_always_inline void php_ilimit_call_cleanup(php_ilimit_call_t *call) { /* {{{ */
    zend_execute_data *execute_data = call->zend.frame,
                      *execute_entry = call->zend.entry;

    while (execute_data && execute_data != execute_entry) {
        zend_execute_data *prev;
        uint32_t info =
            ZEND_CALL_INFO(execute_data);

        zend_vm_stack_free_args(execute_data);

        if (info & ZEND_CALL_FREE_EXTRA_ARGS) {
            zend_vm_stack_free_extra_args_ex(info, execute_data);
        }

        if (EX(func)->type == ZEND_USER_FUNCTION) {

            if (EX(func)->op_array.last_var) {
                zval *var = EX_VAR_NUM(EX(func)->common.num_args),
                     *end = var + (EX(func)->op_array.last_var);

                while (var < end) {
                    if (Z_OPT_REFCOUNTED_P(var)) {
                        zval_ptr_dtor(var);
                    }
                    var++;
                }
            }

            if (EX(func)->op_array.last_live_range) {
                int i;
                uint32_t op = EX(opline) - EX(func)->op_array.opcodes;

                for (i = 0; i < EX(func)->op_array.last_live_range; i++) {
                    const zend_live_range *range = &EX(func)->op_array.live_range[i];

                    if (range->start > op) {
                        break;
                    }

                    if (op < range->end) {
                        uint32_t kind = range->var & ZEND_LIVE_MASK;
                        uint32_t var_num = range->var & ~ZEND_LIVE_MASK;
                        zval *var = EX_VAR(var_num);

                        if (kind == ZEND_LIVE_TMPVAR) {
                            zval_ptr_dtor_nogc(var);
                        } else if (kind == ZEND_LIVE_LOOP) {
                            if (Z_TYPE_P(var) != IS_ARRAY && Z_FE_ITER_P(var) != (uint32_t)-1) {
                                zend_hash_iterator_del(Z_FE_ITER_P(var));
                            }
                            zval_ptr_dtor_nogc(var);
                        } else if (kind == ZEND_LIVE_ROPE) {
                            zend_string **rope = (zend_string **)var;
                            zend_op *last = EX(func)->op_array.opcodes + op;
                            while ((last->opcode != ZEND_ROPE_ADD && last->opcode != ZEND_ROPE_INIT)
                                    || last->result.var != var_num) {
                                ZEND_ASSERT(last >= EX(func)->op_array.opcodes);
                                last--;
                            }
                            if (last->opcode == ZEND_ROPE_INIT) {
                                zend_string_release_ex(*rope, 0);
                            } else {
                                int j = last->extended_value;
                                do {
                                    zend_string_release_ex(rope[j], 0);
                                } while (j--);
                            }
                        } else if (kind == ZEND_LIVE_SILENCE) {
                            if (!EG(error_reporting) && Z_LVAL_P(var) != 0) {
                                EG(error_reporting) = Z_LVAL_P(var);
                            }
                        }
                    }
                }
            }
        }

        if (info & ZEND_CALL_RELEASE_THIS) {
            if (info & ZEND_CALL_CTOR) {
                GC_DELREF(Z_OBJ(EX(This)));
                if (GC_REFCOUNT(Z_OBJ(EX(This))) == 1) {
                    zend_object_store_ctor_failed(Z_OBJ(EX(This)));
                }
            }
            OBJ_RELEASE(Z_OBJ(EX(This)));
        }

        if (EX(func)->common.fn_flags & ZEND_ACC_CLOSURE) {
            OBJ_RELEASE(ZEND_CLOSURE_OBJECT(EX(func)));
        }

        prev = EX(prev_execute_data);

        if (prev != execute_entry) {
            zend_vm_stack_free_call_frame_ex(info, execute_data);
        }

        execute_data = prev;
    }

    EG(current_execute_data) = execute_entry;
} /* }}} */

static zend_always_inline void php_ilimit_call_destroy(php_ilimit_call_t *call) { /* {{{ */
    if (call->state & PHP_ILIMIT_TIMEOUT) {
        zend_throw_exception_ex(php_ilimit_cpu_ex, 0,
            "the time limit of %" PRIu64 " microseconds has been reached",
            call->limits.cpu);
        php_ilimit_call_cleanup(call);
    }

    if (call->state & PHP_ILIMIT_MEMORY) {
        zend_throw_exception_ex(php_ilimit_memory_ex, 0,
            "the memory limit of %" PRIu64 " bytes has been reached",
            call->limits.memory.max);
        php_ilimit_call_cleanup(call);
    }

    pthread_mutex_destroy(&call->mutex);
    pthread_cond_destroy(&call->cond);
} /* }}} */

void php_ilimit_call(php_ilimit_call_t *call) { /* {{{ */
    struct timespec clock;

    pthread_mutex_lock(&call->mutex);

    if (call->limits.cpu <= 0) {
        zend_throw_exception_ex(php_ilimit_runtime_ex, 0,
            "timeout must be positive");
        call->state |= PHP_ILIMIT_FINISHED;
        pthread_mutex_unlock(&call->mutex);

        goto __php_ilimit_call_destroy;
    }

    if (call->limits.memory.max < 0) {
        zend_throw_exception_ex(php_ilimit_runtime_ex, 0,
            "memory must not be negative");
        call->state |= PHP_ILIMIT_FINISHED;
        pthread_mutex_unlock(&call->mutex);

        goto __php_ilimit_call_destroy;
    }

    if (call->limits.memory.max > 0) {
        if (php_ilimit_memory(call) != SUCCESS) {
            zend_throw_exception_ex(php_ilimit_memory_ex, 0,
                "memory limit of %" PRIu64 " bytes would be exceeded",
                PG(memory_limit));
            call->state |= PHP_ILIMIT_FINISHED;
            pthread_mutex_unlock(&call->mutex);

            goto __php_ilimit_call_destroy;
        }

        if (pthread_create(&call->threads.memory, NULL, __php_ilimit_memory_thread, call) != SUCCESS) {
            zend_throw_exception_ex(php_ilimit_sys_ex, 0,
                "cannot create memory management thread");
            call->state |= PHP_ILIMIT_FINISHED;
            pthread_mutex_unlock(&call->mutex);

            goto __php_ilimit_call_destroy;
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

        goto __php_ilimit_call_destroy;
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

                php_ilimit_call_cancel(call);
            }

            goto __php_ilimit_call_finish;
        }
    }

    pthread_mutex_unlock(&call->mutex);

__php_ilimit_call_finish:
    pthread_join(call->threads.cpu, NULL);

    if (call->limits.memory.max) {
        pthread_join(call->threads.memory, NULL);
    }

__php_ilimit_call_destroy:
    php_ilimit_call_destroy(call);
} /* }}} */

#endif
