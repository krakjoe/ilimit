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
#ifndef HAVE_PHP_ILIMIT_H
#define HAVE_PHP_ILIMIT_H

#include "zend_exceptions.h"
#include "zend_closures.h"

#ifdef ZTS
# error "Cannot support thread safe builds"
#else
# include <pthread.h>
#endif

extern zend_class_entry *php_ilimit_ex;
extern zend_class_entry *php_ilimit_sys_ex;
extern zend_class_entry *php_ilimit_cpu_ex;
extern zend_class_entry *php_ilimit_memory_ex;

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
        zend_long cpu;
        struct _memory {
            zend_long max;
            zend_long interval;
        } memory;
    } limits;

} php_ilimit_call_t;

#define PHP_ILIMIT_RUNNING  0x00000001
#define PHP_ILIMIT_FINISHED 0x00000010
#define PHP_ILIMIT_TIMEOUT  0x00000100
#define PHP_ILIMIT_MEMORY   0x00001000
#define PHP_ILIMIT_INTERRUPTED 0x00010000

void php_ilimit_startup(void);

void php_ilimit_call_init(php_ilimit_call_t *call, zend_execute_data *entry);
void php_ilimit_call(php_ilimit_call_t *call);
#endif
