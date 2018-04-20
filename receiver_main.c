/* Server code base taken from Beej's */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <stdint.h>


int timeout_in_ms = 200; //???? what's an appropriate value?
int headersize = 2; //ALSO CHANGE IN SENDER
#define MAXBUFLEN 1472

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, char *argv[])
{

	if(argc != 3)
	{
		fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
		exit(1);
	}

	unsigned short intudpPort = (unsigned short int)atoi(argv[1]);
	char * filepath = argv[2];

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("receiver: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("receiver: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "receiver: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("receiver: waiting to recvfrom...\n");

    FILE * file;
    //this should probably be "a" instead of "w" since we're writing in bursts
    file = fopen(filepath, "a");

//this probably all needs to go in a while loop so it can keep receiving...?

//timeout stuff
struct timeval tv;
tv.tv_sec = 0;
tv.tv_usec = timeout_in_ms * 1000;
if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    //timeout handling....
}

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

      //now send an ACK. In the future, the received packets will be buffered and sorted before getting written.
      //Use selective acknowledgment and ack all frames received

   //these 3 printfs are purely for debugging
    printf("listener: got packet from %s\n",
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s));
    printf("listener: packet is %d bytes long\n", numbytes);
    buf[numbytes + headersize] = '\0'; //*headersize
    //this line doesn't really work anymore with the headers
    //once all packets have been received and put in order,

    //TODO: write starting at a bufindex we keep track of. DO NOT calculate with packet # * packet size, since packet # will wrap around!
    //TODO: change third parameter if header process changes
    fwrite(buf + headersize, 1, numbytes - headersize, file); //*headersize
    //bufindex += packet_size;



    close(sockfd);

    return 0;
}
