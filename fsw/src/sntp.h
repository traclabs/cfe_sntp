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
 * @file
 *
 * Main header file for the SAMPLE application
 */

#ifndef SNTP_H
#define SNTP_H

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_error.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"

#include "sntp_perfids.h"
#include "sntp_msgids.h"
#include "sntp_msg.h"

/***********************************************************************/
#define SNTP_PIPE_DEPTH 32 /* Depth of the Command Pipe for Application */

#define SNTP_NUMBER_OF_TABLES 1 /* Number of Table(s) */

/* Define filenames of default data images for tables */
#define SNTP_TABLE_FILE "/cf/sntp_tbl.tbl"

#define SNTP_TABLE_OUT_OF_RANGE_ERR_CODE -1

#define SNTP_TBL_ELEMENT_1_MAX 10
/************************************************************************
** Type Definitions
*************************************************************************/

/*
** Global Data
*/
typedef struct
{
    /*
    ** Command interface counters...
    */
    SNTP_HkTlm_Payload_t cnts;

    /*
    ** Housekeeping telemetry packet...
    */
    SNTP_HkTlm_t HkTlm;

    /*
    ** Run Status variable used in the main processing loop
    */
    uint32 RunStatus;

    /*
    ** Operational data (not reported in housekeeping)...
    */
    CFE_SB_PipeId_t CommandPipe;

    /*
    ** Initialization data (not reported in housekeeping)...
    */
    char   PipeName[CFE_MISSION_MAX_API_LEN];
    uint16 PipeDepth;

    //CFE_TBL_Handle_t TblHandles[SNTP_NUMBER_OF_TABLES];

    int sockfd;

} SNTP_Data_t;

/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (SNTP_Main), these
**       functions are not called from any other source module.
*/
void  SNTP_Main(void);
int32 SNTP_Init(void);
void  SNTP_ProcessCommandPacket(CFE_SB_Buffer_t *SBBufPtr);
void  SNTP_ProcessGroundCommand(CFE_SB_Buffer_t *SBBufPtr);
int32 SNTP_ReportHousekeeping(const CFE_MSG_CommandHeader_t *Msg);
int32 SNTP_ResetCounters(const SNTP_ResetCountersCmd_t *Msg);
int32 SNTP_Process(const SNTP_ProcessCmd_t *Msg);
int32 SNTP_Noop(const SNTP_NoopCmd_t *Msg);
void  SNTP_GetCrc(const char *TableName);

int32 SNTP_TblValidationFunc(void *TblData);

bool SNTP_VerifyCmdLength(CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength);

#endif /* SNTP_H */
