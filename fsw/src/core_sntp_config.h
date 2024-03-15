/*
 * coreSNTP v1.2.0
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file core_sntp_config.h
 * @brief This header sets configuration macros for the SNTP library.
 */
#ifndef CORE_SNTP_CONFIG_H_
#define CORE_SNTP_CONFIG_H_

/* Standard include. */
#include <stdio.h>

/**
 * @brief Utility to convert fractions part of SNTP timestamp to milliseconds.
 *
 * @param[in] fractions The fractions value in an SNTP timestamp.
 */
#define FRACTIONS_TO_MS( fractions ) \
    ( fractions / ( SNTP_FRACTION_VALUE_PER_MICROSECOND * 1000U ) )

/**
 * @brief Utility macro to convert milliseconds to the fractions value of an SNTP timestamp.
 * @note The fractions value MUST be less than 1000 as duration of seconds is not represented
 * as fractions part of SNTP timestamp.
 */
#define MILLISECONDS_TO_SNTP_FRACTIONS( ms )    ( ms * 1000 * SNTP_FRACTION_VALUE_PER_MICROSECOND )




#define NET_BUF_SIZE SNTP_PACKET_BASE_SIZE // Note Size may increase if auth is added



/* @[code_example_loggingmacros] */
/************* Define Logging Macros using printf function ***********/
#if 0 // TODO: Ifdef cfe
#define PrintfError( ... )         printf( "Error: "__VA_ARGS__ );  printf( "\n" )
#define PrintfWarn( ... )          printf( "Warn: "__VA_ARGS__ );  printf( "\n" )
#define PrintfInfo( ... )          printf( "Info: " __VA_ARGS__ ); printf( "\n" )
#define PrintfDebug( ... )         printf( "Debug: " __VA_ARGS__ ); printf( "\n" )
#else
#define PrintfError( ... )         OS_printf( "Error: "__VA_ARGS__ );  OS_printf( "\n" )
#define PrintfWarn( ... )          OS_printf( "Warn: "__VA_ARGS__ );  OS_printf( "\n" )
#define PrintfInfo( ... )          OS_printf( "Info: " __VA_ARGS__ ); OS_printf( "\n" )
#define PrintfDebug( ... )         OS_printf( "Debug: " __VA_ARGS__ ); OS_printf( "\n" )
#endif

#ifdef LOGGING_LEVEL_ERROR
    #define LogError( message )    PrintfError message
#elif defined( LOGGING_LEVEL_WARNING )
    #define LogError( message )    PrintfError message
    #define LogWarn( message )     PrintfWarn message
#elif defined( LOGGING_LEVEL_INFO )
    #define LogError( message )    PrintfError message
    #define LogWarn( message )     PrintfWarn message
    #define LogInfo( message )     PrintfInfo message
#elif defined( LOGGING_LEVEL_DEBUG )
    #define LogError( message )    PrintfError message
    #define LogWarn( message )     PrintfWarn message
    #define LogInfo( message )     PrintfInfo message
    #define LogDebug( message )    PrintfDebug message
#endif /* ifdef LOGGING_LEVEL_ERROR */

/**************************************************/
/* @[code_example_loggingmacros] */

#endif /* ifndef CORE_SNTP_CONFIG_H_ */
