#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>


#define headersize 4
#define MAXBUFLEN 1472
#define BWDELAY 170
#define NUMSEQNUMS (BWDELAY * 2)
int RWS = 35;
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
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");


        FILE * file;
        //this should probably be "a" instead of "w" since we're writing in bursts
        file = fopen(filepath, "a");




    addr_len = sizeof their_addr;


    uint16_t next_expected_frame = 0;
    uint16_t last_acceptable_frame = (next_expected_frame + RWS - 1) % NUMSEQNUMS;
    struct timeval lastheardtime;
    struct timeval timeout; //2 * RTT = 2 * 20ms
    //20ms = 20,000 microseconds
    timeout.tv_usec = 20000;
    timeout.tv_sec = 1; //for testing purposes, set a high timeout
    //we're seeing differences of 50ish in seconds....
    int i;
    int done = 0;
     char storagebuf[RWS][MAXBUFLEN];
     char receivedbuf[RWS];
     uint32_t lastbufsmaller[RWS];
      for (i = 0; i < RWS; i++){
        lastbufsmaller[i] = 0;
      }
     //NOTE: this will somehow need to be placed in the main while loop
     //without getting overwritten, since RWS will change
     //could make it really long...
      for (i = 0; i < RWS; i++){
        receivedbuf[i] = 0;
      }
      int total_packets_expected = 0;
      int total_packets_received = 0;
      
      int expected_packets_this_window;

    //two conditions: timeout or fill window
    while(1){
      char tempbuf[MAXBUFLEN];
      //a boolean buf to keep track of what's been receieved
      //2D array for the buffer contents
      int i;
    

      if ((numbytes = recvfrom(sockfd, tempbuf, MAXBUFLEN , 0,
          (struct sockaddr *)&their_addr, &addr_len)) == -1) {
          perror("recvfrom1");
          exit(1);
      }
      gettimeofday(&lastheardtime, NULL);
      //parse received packet's sequence numbers
      uint16_t packetnum = 0;
      packetnum = packetnum | tempbuf[1];
      packetnum = packetnum | (tempbuf[0] << 8);
      expected_packets_this_window = (char)tempbuf[2];
      total_packets_expected = tempbuf[3];
      if (expected_packets_this_window > RWS)
        expected_packets_this_window = RWS;
      if(numbytes < MAXBUFLEN)
        lastbufsmaller[packetnum % expected_packets_this_window]  = numbytes - headersize;
      //should it also be less than next expected frame? that would mess up on wrapping around to 0
      //or if this is a duplicate packet
      if ((packetnum > last_acceptable_frame) || receivedbuf[packetnum % expected_packets_this_window]) {
        //drop it
      }

      else{
        //transfer contents into the permanent buf for this packet
        memcpy(storagebuf[packetnum % expected_packets_this_window], tempbuf+headersize, numbytes - headersize);
        //mark this packet number as present
        receivedbuf[packetnum % expected_packets_this_window] = 1;
        total_packets_received++;
      }
      int allpresent = 1;
      if(total_packets_received == total_packets_expected)
      	 done = 1;
      //if all packets in the window are present:
      for (i = 0; i < expected_packets_this_window; i++){
        //check for any instance of not present
        if(receivedbuf[i] == 0){
          allpresent = 0;
          break;
        }
      }

      //if all packets are present, write them in order to the file
      if (allpresent){
        for (i = 0; i < expected_packets_this_window; i++){
          //account for last one being shorter than maxbuflen
          if(lastbufsmaller[i])
             fwrite(storagebuf[i], 1, lastbufsmaller[i], file);
          else
            fwrite(storagebuf[i], 1, MAXBUFLEN - headersize, file);
        }
        //send an ACK and slide the window
        char ackbuf[2];
        ackbuf[0] = (char)(((next_expected_frame + expected_packets_this_window - 1) & 0xFF00) >> 8);
        ackbuf[1] = (char)((next_expected_frame + expected_packets_this_window - 1) & 0x00FF);
        //NOTE: THIS ACK WILL BE THE WRONG NUMBER IF RWS > SWS
          if ((numbytes = sendto(sockfd, ackbuf, 2, 0,
                  p->ai_addr, p->ai_addrlen)) == -1) {
             perror("sender: sendto");
             exit(1);
         }
        
         //slide the window only if we're not completely done
         if(!done){
		 next_expected_frame+=RWS;
		 if(next_expected_frame >= NUMSEQNUMS)
		    next_expected_frame = 0;
	
		 last_acceptable_frame = (next_expected_frame + RWS - 1) % NUMSEQNUMS;
	
		   for (i = 0; i < RWS; i++){
		     receivedbuf[i] = 0;
		    }
          }
          //if finished
          else {
            char ackbuf[2];
        ackbuf[0] = (char)(((next_expected_frame + expected_packets_this_window - 1) & 0xFF00) >> 8);
        ackbuf[1] = (char)((next_expected_frame + expected_packets_this_window - 1) & 0x00FF);
        //NOTE: THIS ACK WILL BE THE WRONG NUMBER IF RWS > SWS?
          if ((numbytes = sendto(sockfd, ackbuf, 2, 0,
                  p->ai_addr, p->ai_addrlen)) == -1) {
             perror("sender: sendto");
             exit(1);
         }
        
            //send final ack
            fclose(file);
            break;
          }
      }
      /* //ignore timeout acks for now
      //else if any packet has timed out that has NOT been received, send ack for last packet received
        else{
          //take timeout to be 2*RTT after most recent packet was received? or where should it start?
          struct timeval now;
          gettimeofday(&now, NULL);
          if (((now.tv_sec - lastheardtime.tv_sec) >= timeout.tv_sec) && ((now.tv_usec - lastheardtime.tv_usec) > timeout.tv_usec)){
            //send the latest thing you've heard
            //loop through received_buf until you find the first 0. Send the packet right before that
            uint16_t lastheard = -1;
            for (i = 0; i < RWS; i++){
              if(receivedbuf[i] == 0){
                if (i == 0)
                  //loop around
                  lastheard = NUMSEQNUMS -1;
                else
                  lastheard = i - 1;
                break;
              }
            }
            //send lastheard
            char ackbuf[2];
            ackbuf[0] = (lastheard & 0xFF00) >> 8;
            ackbuf[1] = (char)(lastheard & 0x00FF);
            //NOTE: THIS ACK WILL BE THE WRONG NUMBER IF RWS > SWS
              if ((numbytes = sendto(sockfd, ackbuf, 2, 0,
                      p->ai_addr, p->ai_addrlen)) == -1) {
                 perror("sender: sendto");
                 exit(1);
             }

          }

        }*/

  }

    printf("listener: got packet from %s\n",
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s));
    //printf("listener: packet is %d bytes long\n", numbytes);
    //buf[numbytes] = '\0';
    //printf("listener: packet contains \"%s\"\n", buf);

    //TODO: write starting at a bufindex we keep track of. DO NOT calculate with packet # * packet size, since packet # will wrap around!
    //TODO: change third parameter if header process changes
    //fwrite(buf + headersize, 1, numbytes - headersize, file); //*headersize

    close(sockfd);

    return 0;
}
