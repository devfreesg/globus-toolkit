/*
 * Portions of this file Copyright 1999-2005 University of Chicago
 * Portions of this file Copyright 1999-2005 The University of Southern California.
 *
 * This file or a portion of this file is licensed under the
 * terms of the Globus Toolkit Public License, found at
 * http://www.globus.org/toolkit/download/license.html.
 * If you redistribute this file, with or without
 * modifications, you must include this notice in the file.
 */

#ifndef GLOBUS_DONT_DOCUMENT_INTERNAL
/**
 * @file globus_gridmap_callout_error.h
 * @author Sam Meder
 *
 * $RCSfile$
 * $Revision$
 * $Date$
 */
#endif

#ifndef GLOBUS_GRIDMAP_CALLOUT_ERROR_H
#define GLOBUS_GRIDMAP_CALLOUT_ERROR_H

#ifndef EXTERN_C_BEGIN
#    ifdef __cplusplus
#        define EXTERN_C_BEGIN extern "C" {
#        define EXTERN_C_END }
#    else
#        define EXTERN_C_BEGIN
#        define EXTERN_C_END
#    endif
#endif

EXTERN_C_BEGIN

#include "globus_common.h"
#include "globus_error_gssapi.h"

/** 
 * @defgroup globus_gridmap_callout_error_activation Activation
 *
 * The Globus Gridmap Callout Error API uses standard Globus module
 * activation and deactivation.  Before any Globus Gridmap Callout
 * Error API functions are called, the following function must be called:
 *
 * @code
 *      globus_module_activate(GLOBUS_GRIDMAP_CALLOUT_ERROR_MODULE)
 * @endcode
 *
 *
 * This function returns GLOBUS_SUCCESS if the Globus Gridmap Callout
 * Error API was successfully initialized, and you are therefore allowed to
 * subsequently call Globus Gridmap Callout Error API functions.
 * Otherwise, an error code is returned, and Globus GSI Credential functions
 * should not be subsequently called. This function may be called multiple
 * times. 
 *
 * To deactivate Globus Gridmap Callout Error API, the following
 * function must be called: 
 *
 * @code
 *    globus_module_deactivate(GLOBUS_GRIDMAP_CALLOUT_ERROR_MODULE)
 * @endcode
 *
 * This function should be called once for each time Globus Gridmap
 * Callout Error API was activated. 
 *
 */

/** Module descriptor
 * @ingroup globus_gridmap_callout_error_activation
 * @hideinitializer
 */
#define GLOBUS_GRIDMAP_CALLOUT_ERROR_MODULE    (&globus_i_gridmap_callout_error_module)

extern 
globus_module_descriptor_t              globus_i_gridmap_callout_error_module;

/**
 * @defgroup globus_gridmap_callout_error_datatypes Datatypes
 */

/**
 * Gridmap Callout Error codes
 * @ingroup globus_gridmap_callout_error_datatypes
 */
typedef enum
{
    /** Gridmap lookup failed */
    GLOBUS_GRIDMAP_CALLOUT_LOOKUP_FAILED = 0,
    /** GSSAPI error */
    GLOBUS_GRIDMAP_CALLOUT_GSSAPI_ERROR = 1,
    /** Buffer too small */
    GLOBUS_GRIDMAP_CALLOUT_BUFFER_TOO_SMALL = 2,
    /** Last marker - never used */
    GLOBUS_GRIDMAP_CALLOUT_ERROR_LAST = 3
}
globus_gridmap_callout_error_t;

extern char * globus_i_gridmap_callout_error_strings[];

#define GLOBUS_GRIDMAP_CALLOUT_ERROR(__RESULT, __TYPE, __ERRSTR)         \
{                                                                        \
    char *                          _tmp_str_ =                          \
        globus_common_create_string __ERRSTR;                            \
    (__RESULT) = globus_error_put(                                       \
        globus_error_construct_error(                                    \
            GLOBUS_GRIDMAP_CALLOUT_ERROR_MODULE,                         \
            (__RESULT) ? globus_error_get(__RESULT) : NULL,              \
            __TYPE,                                                      \
            __FILE__, \
            "Globus Gridmap Callout",                \
            __LINE__, \
            "%s%s%s",                                         \
            globus_i_gridmap_callout_error_strings[__TYPE],              \
            _tmp_str_ ? ": " : "",                                       \
            _tmp_str_ ? _tmp_str_ : ""));                                \
    if(_tmp_str_) free(_tmp_str_);                                       \
}

#define GLOBUS_GRIDMAP_CALLOUT_GSS_ERROR(__RESULT, __MAJOR_STATUS, __MINOR_STATUS) \
    __RESULT = globus_error_put(                                                   \
        globus_error_wrap_gssapi_error(                                            \
            GLOBUS_GRIDMAP_CALLOUT_ERROR_MODULE,                                   \
            __MAJOR_STATUS,                                                        \
            __MINOR_STATUS,                                                        \
            GLOBUS_GRIDMAP_CALLOUT_GSSAPI_ERROR,                                   \
            __FILE__,                                                              \
            "Globus Gridmap Callout",                                              \
            __LINE__,                                                              \
            "%s",                                                                  \
            globus_i_gridmap_callout_error_strings[GLOBUS_GRIDMAP_CALLOUT_GSSAPI_ERROR]))

EXTERN_C_END

#endif
