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


#ifndef GLOBUS_GSI_PROXY_CONSTANTS_H
#define GLOBUS_GSI_PROXY_CONSTANTS_H

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

/**
 * @defgroup globus_gsi_proxy_constants Proxy Constants
 */
/**
 * Proxy Error codes
 * @ingroup globus_gsi_proxy_constants
 */
typedef enum
{
    /** Success - never used */
    GLOBUS_GSI_PROXY_ERROR_SUCCESS = 0,
    /** Invalid proxy handle state */
    GLOBUS_GSI_PROXY_ERROR_WITH_HANDLE = 1,
    /** Invalid proxy handle attributes state */
    GLOBUS_GSI_PROXY_ERROR_WITH_HANDLE_ATTRS = 2,
    /** Error with ASN.1 proxycertinfo structure */
    GLOBUS_GSI_PROXY_ERROR_WITH_PROXYCERTINFO = 3,
    /** Error with ASN.1 proxypolicy structure */
    GLOBUS_GSI_PROXY_ERROR_WITH_PROXYPOLICY = 4,
    /** Error with proxy path length */
    GLOBUS_GSI_PROXY_ERROR_WITH_PATHLENGTH = 5,
    /** Error with the X.509 request structure */
    GLOBUS_GSI_PROXY_ERROR_WITH_X509_REQ = 6,
    /** Error with X.509 structure */
    GLOBUS_GSI_PROXY_ERROR_WITH_X509 = 7,
    /** Error with X.509 extensions */
    GLOBUS_GSI_PROXY_ERROR_WITH_X509_EXTENSIONS = 8,
    /** Error with private key */
    GLOBUS_GSI_PROXY_ERROR_WITH_PRIVATE_KEY = 9,
    /** Error with OpenSSL's BIO handle */
    GLOBUS_GSI_PROXY_ERROR_WITH_BIO = 10,
    /** Error with credential */
    GLOBUS_GSI_PROXY_ERROR_WITH_CREDENTIAL = 11,
    /** Error with credential handle */
    GLOBUS_GSI_PROXY_ERROR_WITH_CRED_HANDLE = 12,
    /** Error with credential handle attributes */
    GLOBUS_GSI_PROXY_ERROR_WITH_CRED_HANDLE_ATTRS = 13,
    /** System error */
    GLOBUS_GSI_PROXY_ERROR_ERRNO = 14,
    /** Unable to set proxy type */
    GLOBUS_GSI_PROXY_ERROR_SETTING_HANDLE_TYPE = 15,
    /** Invalid function parameter */
    GLOBUS_GSI_PROXY_INVALID_PARAMETER = 16,
    /** A error occured while signing the proxy certificate */
    GLOBUS_GSI_PROXY_ERROR_SIGNING = 17,
    /** Last marker - never used */
    GLOBUS_GSI_PROXY_ERROR_LAST = 18
} globus_gsi_proxy_error_t;

EXTERN_C_END

#endif
