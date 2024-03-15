#ifndef __SNTP_UTILS__
#define __SNTP_UTILS__

#include "core_sntp_serializer.h"

const char* sntp_util_status_to_str(SntpStatus_t status);
void getCurrentSntpTime( SntpTimestamp_t *sntp );

static inline void encodeTime( const SntpTimestamp_t *in, SntpTimestamp_t *out ) {
    out->seconds = htonl(in->seconds);
    out->fractions = htonl(in->fractions);
}

SntpStatus_t Sntp_DeserializeRequest( const void * buf,
                                      SntpPacket_t* request    
    );

#endif
