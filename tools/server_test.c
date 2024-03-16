/*
 * This is a quick demo test server.
 *
 * This version omits cryptographic setup at this time (not needed for onboard time)
 */
#include <assert.h> // For testing
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "core_sntp_serializer.h"
#include "core_sntp_config.h"
#include "sntp_utils.h"

// Glboals
uint8_t netBuf[NET_BUF_SIZE];
int sockfd = -1;

// Command-line Argument Parsing
typedef struct {
    uint16_t port;
    uint8_t  stratum;
} server_args_t;

server_args_t server_args = {
    .port = 123,
    .stratum = 15
};

// Function to parse command-line arguments and override struct values
void printUsage() {
    printf("Usage: program_name [options]\n");
    printf("Options:\n");
    printf("  -p, --port <port_number>     Set the server port (default: 123)\n");
    printf("  -s, --stratum <startum>         Set the NTP Stratum level (default: 15)\n");
    printf("  --help                       Display this help message\n");
}
void parseCommandLineArgs(int argc, char* argv[]) {
    for (int i = 1; i < argc; i += 2) {
	if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                printUsage();
                exit(EXIT_SUCCESS);
	} else if (i + 1 < argc) {
	    if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
                server_args.port = atoi(argv[i + 1]);
            } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--stratum") == 0) {
                server_args.stratum = atoi(argv[i + 1]);
            } else {
                fprintf(stderr, "Unknown option: %s\n", argv[i]);
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Unknown option or missing value for option: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }
    printf("Configuration:\n\t Server Port: %d\n\t Stratum: %d \n",
	   server_args.port, server_args.stratum);
}


/** Initialize socket */
void initUDPSocket() {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Bind to server port
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server_args.port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
	perror("Error binding to server port");
	close(sockfd);
	exit(EXIT_FAILURE);
    }

    return;
}

/** Perform a single query of the SNTP server and await a response */
SntpStatus_t run_sntp_server() {
    SntpStatus_t status;
    SntpPacket_t request, response;
    SntpTimestamp_t time;
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    memset(&response,0,sizeof(response));
    
    // Wait on response (or timeout) and validate size.  Optional retry if read fails
    // Receive response from the server
    ssize_t receivedBytes = recvfrom(sockfd, netBuf, NET_BUF_SIZE, 0, (struct sockaddr*)&clientAddr, &clientLen);
    if (receivedBytes < 0) {
        perror("Error receiving data");
	return SntpErrorNetworkFailure;
    } else if (receivedBytes == 0) {
	perror("No data received");
	return SntpNoResponseReceived;
    } else if (receivedBytes != NET_BUF_SIZE ) {
	printf("Received unexpected number of bytes %zi\n", receivedBytes);
	return SntpErrorNetworkFailure;
    }
    printf("Received NTP Request\n");

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
    response.stratum = server_args.stratum;
    response.refId = htonl(SNTP_KISS_OF_DEATH_CODE_NONE);
    
    getCurrentSntpTime(&time);
    encodeTime(&time, &response.transmitTime);
    
    if (sendto(sockfd, &response, sizeof(response), 0,
               (struct sockaddr*)&clientAddr, clientLen) < 0)
    {
        printf("ERROR: Unable to send reply\n");
        return SntpErrorNetworkFailure;
    }

    return SntpSuccess;
}

int main( int argc, char *argv[] )
{
    printf("SNTP Server Test App\n");
    parseCommandLineArgs(argc, argv);

    // Initialize pseudo-random number generator
    srand((unsigned int)time(NULL)); // TODO: Is this needed?
    
    // Create UDP Socket (to be reused for all requests/responses)
    initUDPSocket();

    // Initial test: Only query server once
    printf("SNTP Server Listening\n");
    while(1) {
	run_sntp_server();
    }
    printf("Done\n");
}

