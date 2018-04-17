#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>


void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
	//the sockfd has been set up with hostname and hostUDPport
	/*
	FILE * file;
	file = fopen(filename, "r");
	char filecontents[bytesToTransfer];
	memcpy(filecontents, file,bytesToTransfer);
	if (sendto(sockfd, filecontents, bytesToTransfer, 0) == -1){
		perror("send");
	}
	*/
}

int main(int argc, char** argv)
{
	unsigned short int udpPort;
	unsigned long long int numbytes;
	 int sockfd;
         struct addrinfo hints, *servinfo, *p;
         int rv;
        // int numbytes;
	
	if(argc != 5)
	{
		fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
		exit(1);
	}
	udpPort = (unsigned short int)atoi(argv[2]);
	numbytes = atoll(argv[4]);
	
	 memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    //you want to send the file contents
    char * filepath = argv[3];
    FILE * file;
    file = fopen(filepath, "r");
    char c;
    char buf[numbytes];
    int count = 0;
    while(c = fgetc(file) !=EOF && count < numbytes){
    	buf[count] = c;
    	count++;
    }
    if ((numbytes = sendto(sockfd, buf, numbytes, 0,
             p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes to %s\n", (int)numbytes, argv[1]);
    close(sockfd);

    return 0;
}
