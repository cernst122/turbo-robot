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

uint16_t mynum = 0;
int headersize = 2; //ALSO CHANGE IN RECEIVER

#define packet_size 1472 //1472 B packets
#define BWDELAY 1359 //100*1000000 * .02 / packet_size = 1359  //100Mbps * 20ms / packet_size
//BWD should dynamically adjust based on RTT, maybe? or will RTT stay fairly constant?
//timeout will be 2 * RTT ish

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
            perror("sender: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "sender: failed to create socket\n");
        return 2;
    }

    //you want to send the file contents
    char * filepath = argv[3];
    FILE * file;
    file = fopen(filepath, "r");
    if (!file){
    	perror("file not found");
    	exit(1);
    }

    char c;
    char megabuf[numbytes];
    int count = 0;
    //reserve a few bytes of header at the beginning for packet number.
    //Size of header should be enough to represent the total sequence numbers.
    //Start with window up to 65535? 16 bits = 2 bytes
    int headersize = 2;
    while(((c = fgetc(file)) !=EOF) && count < numbytes){
    	megabuf[count + headersize] = c;
    	count++;
    }
		//This should all be copied at the beginning, and as we send more packets, just send buf starting at packet num * 1472.
		//But then how do we attach headers?

		//Attach two-byte header info with my packet number
		int packets_sent = 0;
		int total_packets = int(numbytes / (packet_size - headersize)) + 1;
    while(packets_sent < total_packets){
			char buf[packet_size];
	    //mynum is 16 bits.
	    buf[0] = (mynum & 0xFF00) >> 8;
	    buf[1] = (char)(mynum & 0x00FF);
			memcpy(buf+headersize, megabuf+packets_sent*(packet_size-headersize), packet_size-headersize);
			//if last packet, only send a a certain number of bytes
			if(packets_sent = total_packets -1){
				int bytes_to_send = numbytes % (packet_size = headersize);
				if ((numbytes = sendto(sockfd, buf, bytes_to_send, 0,
								p->ai_addr, p->ai_addrlen)) == -1) {
					 perror("sender: sendto");
					 exit(1);
			 }
			}

			//if not last packet, send normal packet size
			else{
		    if ((numbytes = sendto(sockfd, buf, packet_size, 0,
		             p->ai_addr, p->ai_addrlen)) == -1) {
		        perror("sender: sendto");
		        exit(1);
		    }
			}


			//TODO: This will need to be modulo total sequence numbers
	    mynum++;
			packets_sent++;
			//TODO: Wait until necessary ACKS received before continuing to send/sliding window. If not, reduce my_num and packets_sent to resend earlier information.
	  }
    //need to keep track of all old transmissions until the ACK has been received
    //now needs some mechanism to receive ACKs

		//record time when ACK is received to estimate RTT


    freeaddrinfo(servinfo);

    printf("sender: sent %d bytes to %s\n", (int)numbytes, argv[1]);
    close(sockfd);

    return 0;
}
