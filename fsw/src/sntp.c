/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * \file
 *   This file contains the source code for the SNTP App.
 */

/*
** Include Files:
*/
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>

#include "sntp_events.h"
#include "sntp_version.h"
#include "sntp.h"
#include "sntp_table.h"

#include "core_sntp_serializer.h"
#include "core_sntp_config.h"
#include "sntp_utils.h"

#ifndef SNTP_PORT
#define SNTP_PORT 123
#endif
#ifndef SNTP_STRATUM
#define SNTP_STRATUM 15
#endif

/*
** global data
*/
SNTP_Data_t SNTP_Data;
uint8_t netBuf[NET_BUF_SIZE];

/** Initialize socket */
int initUDPSocket(uint32_t port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
	return sockfd;
    }

    // Bind to server port
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SNTP_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
	perror("Error binding to server port");
	close(sockfd);
	return -1;
    }

    // Set 1s timeout to allow for periodic polling for cFE Commands
    struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(struct timeval));

    return sockfd;
}

/** Process a single query */
SntpStatus_t process_sntp_request(struct sockaddr_in *clientAddr, socklen_t clientLen) {
    SntpStatus_t status;
    SntpPacket_t request, response;
    SntpTimestamp_t time;
    memset(&response,0,sizeof(response));
    
    // Receive Time when response is received. Used to calculate system clock offset
    getCurrentSntpTime(&time);
    encodeTime(&time, &response.receiveTime);

    // NOTE: Skip auth decoding

    // De-serialize packet
    status = Sntp_DeserializeRequest( netBuf, &request);
    if (status != SntpSuccess) {
        printf("ERROR: Invalid request\n");
        return status;
    }

    // Echo request in response fields
    encodeTime( &request.transmitTime, &response.originTime );

    // Set Details
    response.leapVersionMode = SNTP_MODE_SERVER | ( SNTP_VERSION << SNTP_VERSION_LSB_POSITION );
    response.stratum = SNTP_STRATUM;
    response.refId = htonl(SNTP_KISS_OF_DEATH_CODE_NONE);
    
    getCurrentSntpTime(&time);
    encodeTime(&time, &response.transmitTime);
    
    if (sendto(SNTP_Data.sockfd, &response, sizeof(response), 0,
               (struct sockaddr*)clientAddr, clientLen) < 0)
    {
        printf("ERROR: Unable to send reply\n");
        return SntpErrorNetworkFailure;
    }

    return SntpSuccess;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
/* SNTP_Main() -- Application entry point and main process loop         */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void SNTP_Main(void)
{
    int32            status;
    CFE_SB_Buffer_t *SBBufPtr;
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    /*
    ** Create the first Performance Log entry
    */
    CFE_ES_PerfLogEntry(SNTP_PERF_ID);

    /*
    ** Perform application specific initialization
    ** If the Initialization fails, set the RunStatus to
    ** CFE_ES_RunStatus_APP_ERROR and the App will not enter the RunLoop
    */
    status = SNTP_Init();
    if (status != CFE_SUCCESS)
    {
        OS_printf("SNTP Init Failed!\n");
        SNTP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    /*
    ** SNTP Runloop
    */
    while (CFE_ES_RunLoop(&SNTP_Data.RunStatus) == true)
    {
        /*
        ** Performance Log Exit Stamp
        */
        CFE_ES_PerfLogExit(SNTP_PERF_ID);

        /* Poll for receipt of command packet */
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, SNTP_Data.CommandPipe, CFE_SB_POLL);

        /*
        ** Performance Log Entry Stamp
        */
        CFE_ES_PerfLogEntry(SNTP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            OS_printf("SNTP CMD Received\n");
            SNTP_ProcessCommandPacket(SBBufPtr);
        }

        /* Wait on receipt of UDP Packet, with 1s timeout for periodic checking */
        ssize_t receivedBytes = recvfrom(SNTP_Data.sockfd, netBuf, NET_BUF_SIZE, 0, (struct sockaddr*)&clientAddr, &clientLen);

        if (receivedBytes == NET_BUF_SIZE ) {
            process_sntp_request(&clientAddr, clientLen);
        } else if (receivedBytes > 0 ) {
            OS_printf("ERROR: Invalid packet received of size %li\n", receivedBytes);
        } else if (receivedBytes < 0 && errno != ETIMEDOUT && errno != EAGAIN ) {
            OS_printf("Unexpected recvfrom error: %li %i=%s\n", receivedBytes, errno, strerror(errno));
        } // else probable timeout
        
    }

    OS_printf("****SNTP App Exiting****\n");

    /*
    ** Performance Log Exit Stamp
    */
    CFE_ES_PerfLogExit(SNTP_PERF_ID);

    CFE_ES_ExitApp(SNTP_Data.RunStatus);

} /* End of SNTP_Main() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* SNTP_Init() --  initialization                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
int32 SNTP_Init(void)
{
    int32 status;

    SNTP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    /*
    ** Initialize app command execution counters
    */
    SNTP_Data.CmdCounter = 0;
    SNTP_Data.ErrCounter = 0;

    /*
    ** Initialize app configuration data
    */
    SNTP_Data.PipeDepth = SNTP_PIPE_DEPTH;

    strncpy(SNTP_Data.PipeName, "SNTP_CMD_PIPE", sizeof(SNTP_Data.PipeName));
    SNTP_Data.PipeName[sizeof(SNTP_Data.PipeName) - 1] = 0;

    /*
    ** Register the events
    */
    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("SNTP App: Error Registering Events, RC = 0x%08lX\n", (unsigned long)status);
        return (status);
    }

    /*
    ** Initialize housekeeping packet (clear user data area).
    */
    CFE_MSG_Init(CFE_MSG_PTR(SNTP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(SNTP_HK_TLM_MID),
                 sizeof(SNTP_Data.HkTlm));

    /*
    ** Create Software Bus message pipe.
    */
    status = CFE_SB_CreatePipe(&SNTP_Data.CommandPipe, SNTP_Data.PipeDepth, SNTP_Data.PipeName);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("SNTP App: Error creating pipe, RC = 0x%08lX\n", (unsigned long)status);
        return (status);
    }

    /*
    ** Subscribe to Housekeeping request commands
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(SNTP_SEND_HK_MID), SNTP_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("SNTP App: Error Subscribing to HK request, RC = 0x%08lX\n", (unsigned long)status);
        return (status);
    }

    /*
    ** Subscribe to ground command packets
    */
    status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(SNTP_CMD_MID), SNTP_Data.CommandPipe);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("SNTP App: Error Subscribing to Command, RC = 0x%08lX\n", (unsigned long)status);

        return (status);
    }

    SNTP_Data.sockfd = initUDPSocket(SNTP_PORT);
    
    CFE_EVS_SendEvent(SNTP_STARTUP_INF_EID, CFE_EVS_EventType_INFORMATION, "SNTP App Initialized.%s",
                      SNTP_VERSION_STRING);

    return (CFE_SUCCESS);

} /* End of SNTP_Init() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  SNTP_ProcessCommandPacket                                    */
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the SNTP    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void SNTP_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    switch (CFE_SB_MsgIdToValue(MsgId))
    {
        case SNTP_CMD_MID:
            SNTP_ProcessGroundCommand(SBBufPtr);
            break;

        case SNTP_SEND_HK_MID:
            SNTP_ReportHousekeeping((CFE_MSG_CommandHeader_t *)SBBufPtr);
            break;

        default:
            CFE_EVS_SendEvent(SNTP_INVALID_MSGID_ERR_EID, CFE_EVS_EventType_ERROR,
                              "SNTP: invalid command packet,MID = 0x%x", (unsigned int)CFE_SB_MsgIdToValue(MsgId));
            break;
    }

    return;

} /* End SNTP_ProcessCommandPacket */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* SNTP_ProcessGroundCommand() -- SNTP ground commands                */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void SNTP_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    /*
    ** Process "known" SNTP app ground commands
    */
    switch (CommandCode)
    {
        case SNTP_NOOP_CC:
            if (SNTP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(SNTP_NoopCmd_t)))
            {
                SNTP_Noop((SNTP_NoopCmd_t *)SBBufPtr);
            }

            break;

        case SNTP_RESET_COUNTERS_CC:
            if (SNTP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(SNTP_ResetCountersCmd_t)))
            {
                SNTP_ResetCounters((SNTP_ResetCountersCmd_t *)SBBufPtr);
            }

            break;

        /* default case already found during FC vs length test */
        default:
            CFE_EVS_SendEvent(SNTP_COMMAND_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Invalid ground command code: CC = %d", CommandCode);
            break;
    }

    return;

} /* End of SNTP_ProcessGroundCommand() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  SNTP_ReportHousekeeping                                          */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 SNTP_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg)
{
    /*
    ** Get command execution counters...
    */
    SNTP_Data.HkTlm.Payload.CommandErrorCounter = SNTP_Data.ErrCounter;
    SNTP_Data.HkTlm.Payload.CommandCounter      = SNTP_Data.CmdCounter;

    /*
    ** Send housekeeping telemetry packet...
    */
    CFE_SB_TimeStampMsg(CFE_MSG_PTR(SNTP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(SNTP_Data.HkTlm.TelemetryHeader), true);

    
    return CFE_SUCCESS;

} /* End of SNTP_ReportHousekeeping() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* SNTP_Noop -- SNTP NOOP commands                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
int32 SNTP_Noop(const SNTP_NoopCmd_t *Msg)
{

    SNTP_Data.CmdCounter++;

    CFE_EVS_SendEvent(SNTP_COMMANDNOP_INF_EID, CFE_EVS_EventType_INFORMATION, "SNTP: NOOP command %s",
                      SNTP_APP_VERSION);

    return CFE_SUCCESS;

} /* End of SNTP_Noop */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  SNTP_ResetCounters                                               */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
int32 SNTP_ResetCounters(const SNTP_ResetCountersCmd_t *Msg)
{

    SNTP_Data.CmdCounter = 0;
    SNTP_Data.ErrCounter = 0;

    CFE_EVS_SendEvent(SNTP_COMMANDRST_INF_EID, CFE_EVS_EventType_INFORMATION, "SNTP: RESET command");

    return CFE_SUCCESS;

} /* End of SNTP_ResetCounters() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* SNTP_VerifyCmdLength() -- Verify command packet length                   */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
bool SNTP_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result       = true;
    size_t            ActualLength = 0;
    CFE_SB_MsgId_t    MsgId        = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t FcnCode      = 0;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(SNTP_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode, (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;

        SNTP_Data.ErrCounter++;
    }

    return (result);

} /* End of SNTP_VerifyCmdLength() */

