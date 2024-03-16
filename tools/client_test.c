// TODO: Reverted LogMsg to printf, but def is included in headers after all
// TODO: Add verbosity outputs. Consider run-time configurable logging_levels

/*
 * This is a quick demo test client.
 * This is a simplified client utilizing the serializer directly, foregoing the client lib
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
struct sockaddr_in serverAddr; // We will only support a single server in this implementation, cache it

// Command-line Argument Parsing
typedef struct {
    char serverName[64];
    char serverIP[64];
    uint16_t port;
    uint16_t client_port;
} client_args_t;

client_args_t client_args = {
    .serverName = "localhost",
    .serverIP = "127.0.0.1",
    .port = 123,
    .client_port = 0
};

// Function to parse command-line arguments and override struct values
void printUsage(void) {
    printf("Usage: program_name [options]\n");
    printf("Options:\n");
    printf("  -s, --server <server_name>   Set the server name (default: localhost)\n");
    printf("  -ip, --ip <server_ip>        Set the server IP (default: 127.0.0.1)\n");
    printf("  -p, --port <port_number>     Set the server port (default: 123)\n");
    printf("  -cp, --client-port <client_port_number> Set the client port (default: 0)\n");
    printf("  --help                       Display this help message\n");
}
void parseCommandLineArgs(int argc, char* argv[]) {
    for (int i = 1; i < argc; i += 2) {
	if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                printUsage();
                exit(EXIT_SUCCESS);
	} else if (i + 1 < argc) {
            if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--server") == 0) {
                strcpy(client_args.serverName, argv[i + 1]);
            } else if (strcmp(argv[i], "-ip") == 0 || strcmp(argv[i], "--ip") == 0) {
                strcpy(client_args.serverIP, argv[i + 1]);
            } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
                client_args.port = atoi(argv[i + 1]);
            } else if (strcmp(argv[i], "-cp") == 0 || strcmp(argv[i], "--client-port") == 0) {
                client_args.client_port = atoi(argv[i + 1]);
            } else {
                fprintf(stderr, "Unknown option: %s\n", argv[i]);
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Unknown option or missing value for option: %s\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }
    printf("Configuration:\n\t Server Name: %s\n\t Server IP: %s \n\t Port: %d \n",
	   client_args.serverName, client_args.serverIP, client_args.port);
    if (client_args.client_port != 0) {
	printf("\t Client (listen) port: %d\n", client_args.client_port);
    }
}

/** Initialize socket */
void initUDPClientSocket(void) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Server address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(client_args.port);
    inet_pton(AF_INET, client_args.serverIP, &serverAddr.sin_addr);

    // Bind to client port if defined (non-zero)
    if (client_args.client_port != 0) {
        struct sockaddr_in clientAddr;
        memset(&clientAddr, 0, sizeof(clientAddr));
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_port = htons(client_args.client_port);
        clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(sockfd, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0) {
            perror("Error binding to client port");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }

    return;
}

/** Perform a single query of the SNTP server and await a response */
SntpStatus_t run_sntp_query(void) {
    uint32_t randomNumber = rand();
    SntpResponseData_t parsedResponse;

    // Get Current System Time (in NTP Epoch)
    SntpTimestamp_t requestTime, responseTime;
    getCurrentSntpTime(&requestTime);

    SntpStatus_t status = Sntp_SerializeRequest( &requestTime,
						 randomNumber,
						 netBuf,
						 NET_BUF_SIZE
	);
    assert( status == SntpSuccess );

    // NOTE: To support authentication, it would be appended to buffer now

    // Send data to the server
    ssize_t sentBytes = sendto(sockfd, netBuf, NET_BUF_SIZE, 0,
                               (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    if (sentBytes < 0) {
        perror("Error sending data");
	return SntpErrorNetworkFailure;
    }
    printf("NTP Request Sent\n");

    // Wait on response (or timeout) and validate size.  Optional retry if read fails
    // Receive response from the server
    ssize_t receivedBytes = recvfrom(sockfd, netBuf, NET_BUF_SIZE, 0, NULL, NULL); // TODO: Verify this is blocking
    if (receivedBytes < 0) {
        perror("Error receiving data");
	return SntpErrorNetworkFailure;
    } else if (receivedBytes == 0) {
	perror("No response received");
	return SntpNoResponseReceived;
    } else if (receivedBytes != NET_BUF_SIZE ) {
	printf("Received unexpected number of bytes %li\n", receivedBytes);
	return SntpErrorNetworkFailure;
    }
    printf("Received NTP response\n");

    // Receive Time when response is received. Used to calculate system clock offset
    getCurrentSntpTime(&responseTime);

    // NOTE: Skip auth decoding

    // De-serialize packet
    status = Sntp_DeserializeResponse( &requestTime,
				       &responseTime,
				       netBuf,
				       receivedBytes,
				       &parsedResponse
	);
				       
    
    // Validate parsed packet and process
    // TODO; see core_sntp_client.c:690+

    // If successful, "set time".  For demo purposes, we will simply output time data (and comparison to current)
    if (status == SntpSuccess) {
	printf( "Updating system time TODO: ServerTime=%lu %lums ClockOffset=%lds\n",
                        ( unsigned long ) parsedResponse.serverTime.seconds,
                        ( unsigned long ) FRACTIONS_TO_MS( parsedResponse.serverTime.fractions ),
                        /* Print out in seconds instead of Ms to account for C90 lack of %lld */
                        ( long ) parsedResponse.clockOffsetMs / 1000
	    );

    } else {
	printf("SNTP Response Failed. Status was %i=%s\n", status, sntp_util_status_to_str(status));
    }
    return status;
}

int main( int argc, char *argv[] )
{
    printf("SNTP Client Test App\n");
    parseCommandLineArgs(argc, argv);

    // Initialize pseudo-random number generator
    srand((unsigned int)time(NULL));
    
    // Create UDP Socket (to be reused for all requests/responses)
    initUDPClientSocket();

    // Initial test: Only query server once
    printf("SNTP Client Demo Query\n");
    run_sntp_query();
    printf("Done\n");
}

