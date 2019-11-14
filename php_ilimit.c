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

#include "src/ilimit.h"

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(ilimit)
{
    php_ilimit_startup();

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

/* {{{ arg info */
ZEND_BEGIN_ARG_INFO_EX(zend_ilimit_arginfo, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, callable, IS_CALLABLE, 0)
    ZEND_ARG_TYPE_INFO(0, arguments, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, cpu, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, memory, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, interval, IS_LONG, 0)
ZEND_END_ARG_INFO() /* }}} */

/* {{{ proto mixed \ilimit\call(callable $function [, array $args, int $timeoutMS, int $memoryBytes, int $intervalMs = 100]) */
ZEND_NAMED_FUNCTION(zend_ilimit_call)
{
    php_ilimit_call_t call;
    zval *args = NULL;

    php_ilimit_call_init(&call, execute_data);

    ZEND_PARSE_PARAMETERS_START(1, 5)
        Z_PARAM_FUNC(call.zend.info, call.zend.cache)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(args)
        Z_PARAM_LONG(call.limits.timeout)
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
} /* }}} */

/* {{{ zend_ilimit_functions[]
 */
static const zend_function_entry zend_ilimit_api[] = {
    ZEND_NS_FENTRY("ilimit", call, zend_ilimit_call, zend_ilimit_arginfo, 0)
    ZEND_FE_END
};
/* }}} */

/* {{{ ilimit_module_entry
 */
zend_module_entry ilimit_module_entry = {
    STANDARD_MODULE_HEADER,
    "ilimit",
    zend_ilimit_api,
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
