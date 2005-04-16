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


#include "globus_i_gsi_proxy.h"
#include "globus_gsi_proxy_constants.h"

#ifndef GLOBUS_DONT_DOCUMENT_INTERNAL

static char * 
globus_l_gsi_proxy_error_strings[GLOBUS_GSI_PROXY_ERROR_LAST] =
{

/* 0 */   "Success",
/* 1 */   "Error with the proxy handle",
/* 2 */   "Error with the proxy handle attributes",
/* 3 */   "Error with ASN1 proxycertinfo structure",
/* 4 */   "Error with ASN1 proxypolicy structure",
/* 5 */   "Error with proxy path length",
/* 6 */   "Error with X509 request structure",
/* 7 */   "Error with X509 structure",
/* 8 */   "Error with X509 extensions",
/* 9 */   "Error with private key",
/* 10 */  "Error with openssl's BIO handle",
/* 11 */  "Error with credential",
/* 12 */  "Error with credential handle",
/* 13 */  "Error with credential handle attributes",
/* 14 */  "System error",
/* 15 */  "Unable to set proxy type",
/* 16 */  "Invalid parameter",
/* 17 */  "Error signing proxy certificate"
};

globus_result_t
globus_i_gsi_proxy_openssl_error_result(
    int                                 error_type,
    const char *                        filename,
    const char *                        function_name,
    int                                 line_number,
    const char *                        short_desc,
    const char *                        long_desc)
{
    globus_object_t *                   error_object;
    globus_result_t                     result;

    static char *                       _function_name_ =
        "globus_i_gsi_proxy_openssl_error_result";

    GLOBUS_I_GSI_PROXY_DEBUG_ENTER;

    error_object =
        globus_error_wrap_openssl_error(
            GLOBUS_GSI_PROXY_MODULE,
            error_type,
            filename,
            function_name,
            line_number,
            "%s%s%s",
            _PCSL(globus_l_gsi_proxy_error_strings[error_type]),
            short_desc ? ": " : "",
            short_desc ? short_desc : "");
    
    if(long_desc)
    {
        globus_error_set_long_desc(error_object, long_desc);
    }

    result = globus_error_put(error_object);

    GLOBUS_I_GSI_PROXY_DEBUG_EXIT;
    return result;
}
        
globus_result_t
globus_i_gsi_proxy_error_result(
    int                                 error_type,
    const char *                        filename,
    const char *                        function_name,
    int                                 line_number,
    const char *                        short_desc,
    const char *                        long_desc)
{
    globus_object_t *                   error_object;
    globus_result_t                     result;

    static char *                       _function_name_ =
        "globus_i_gsi_proxy_error_result";

    GLOBUS_I_GSI_PROXY_DEBUG_ENTER;

    error_object = globus_error_construct_error(
        GLOBUS_GSI_PROXY_MODULE,
        NULL,
        error_type,
        filename,
        function_name,
        line_number, 
        "%s%s%s",
        _PCSL(globus_l_gsi_proxy_error_strings[error_type]),
        short_desc ? ": " : "",
        short_desc ? short_desc : "");

    if(long_desc)
    {
        globus_error_set_long_desc(error_object, long_desc);
    }

    result = globus_error_put(error_object);

    GLOBUS_I_GSI_PROXY_DEBUG_EXIT;
    return result;
}
        
globus_result_t
globus_i_gsi_proxy_error_chain_result(
    globus_result_t                     chain_result,
    int                                 error_type,
    const char *                        filename,
    const char *                        function_name,
    int                                 line_number,
    const char *                        short_desc,
    const char *                        long_desc)
{
    globus_result_t                     result;
    globus_object_t *                   error_object;
    
    static char *                       _function_name_ =
        "globus_i_gsi_proxy_error_chain_result";

    GLOBUS_I_GSI_PROXY_DEBUG_ENTER;

    error_object = 
        globus_error_construct_error(
            GLOBUS_GSI_PROXY_MODULE,
            globus_error_get(chain_result),
            error_type,
            filename,
            function_name,
            line_number, 
            "%s%s%s",
            _PCSL(globus_l_gsi_proxy_error_strings[error_type]),
            short_desc ? ": " : "",
            short_desc ? short_desc : "");
        
    if(long_desc)
    {
        globus_error_set_long_desc(error_object, long_desc);
    }

    result = globus_error_put(error_object);

    GLOBUS_I_GSI_PROXY_DEBUG_EXIT;
    return result;
}

#endif /* GLOBUS_DONT_DOCUMENT_INTERNAL */
