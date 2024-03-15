#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>

#include "core_sntp_config.h"
#include "core_sntp_serializer.h"

const char* sntp_util_status_to_str(SntpStatus_t status) {
    static const char* SntpStatusStrs[] = {
	"SntpSuccess",
	"SntpErrorBadParameter",
	"SntpRejectedResponse",
	"SntpRejectedResponseChangeServer",
	"SntpRejectedResponseRetryWithBackoff",
	"SntpRejectedResponseOtherCode",
	"SntpErrorBufferTooSmall",
	"SntpInvalidResponse",
	"SntpZeroPollInterval",
	"SntpErrorTimeNotSupported",
	"SntpErrorDnsFailure",
	"SntpErrorNetworkFailure",
	"SntpServerNotAuthenticated",
	"SntpErrorAuthFailure",
	"SntpErrorSendTimeout",
	"SntpErrorResponseTimeout",
	"SntpNoResponseReceived",
	"SntpErrorContextNotInitialized"
    };

// Ensure the enum value is within bounds
    if (status < 0 || status >= sizeof(SntpStatusStrs) / sizeof(SntpStatusStrs[0])) {
        return "Unknown SNTP status";
    }

    return SntpStatusStrs[status];
}


/*** System Utility Functions - NOTICE: These functions may vary by target system and may be split into a discrete file later ***/
void getCurrentSntpTime( SntpTimestamp_t *sntp ) {
    // TODO
    struct timespec currentTime;
    if (clock_gettime(CLOCK_REALTIME, &currentTime) == -1) {
        perror("Error getting current time");
        return;
    }

    sntp->seconds = currentTime.tv_sec + SNTP_TIME_AT_UNIX_EPOCH_SECS;
    sntp->fractions = MILLISECONDS_TO_SNTP_FRACTIONS( currentTime.tv_nsec / 1000000 );
}


/** Sntp Query deserializer
 * @param [in] buf - Input buffer
 * @param [out] - Pointer to a struct to decode buffer into
 */
SntpStatus_t Sntp_DeserializeRequest( const void * buf,
                                      SntpPacket_t* request    
    )
{
    const SntpPacket_t* input = (SntpPacket_t*)buf;
    if (request == NULL) {
        return SntpErrorBadParameter;
    }
    request->leapVersionMode = input->leapVersionMode; // Endian-neutral
    request->transmitTime.seconds = ntohl( input->transmitTime.seconds );
    request->transmitTime.fractions = ntohl( input->transmitTime.fractions );
    return SntpSuccess;
}

