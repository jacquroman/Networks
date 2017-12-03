#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define SERVER_PORT 47500
#define MAX_LINE 256
#define FLAG_HELLO		((unsigned char)(0x01 << 7))
#define FLAG_INSTRUCTION 	((unsigned char)(0x01 << 6))
#define FLAG_RESPONSE		((unsigned char)(0x01 << 5))
#define FLAG_TERMINATE 		((unsigned char)(0x01 << 4))

#define PUSH                    ((unsigned char)(0x03))
#define DIGEST                  ((unsigned char)(0x04))

int main(){
   FILE *fp;
   struct hostent *hp;
   struct sockaddr_in sin;
   char *host;
   int s;

   host = "localhost";
   
   /* translate host name into peer's IP address */
   
   hp = gethostbyname(host);
   if (!hp) {
      fprintf(stderr, "simplex-talk: unknown host: %s\n", host);
      exit(1);
   }
 
   /* build address data structure */
   
   bzero((char *)&sin, sizeof(sin));
   sin.sin_family = AF_INET;
   bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
   sin.sin_port = htons(SERVER_PORT);
   
   /* active open */

   if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
      perror("simplex-talk: socket");
      exit(1);
   }
   if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
      perror("simplex-talk: connect");
      close(s);
      exit(1);
   }

   /*Define the packet*/   
   struct hw_packet{
	unsigned char flag; // HIRT-4bits, reserved-4bits
	unsigned char operation; // 8-bits operation
	unsigned short data_len; // 16 bits (2 bytes) data length
	unsigned int seq_num; //32 bits (4 bytes) sequence number
	char data[1024]; // optional data
   };

   /*Declare variables and assign to structure*/
   unsigned int value;
   struct hw_packet buf_struct;
   buf_struct.flag = FLAG_HELLO;
   //buf_struct.operation = OP_ECHO;
   buf_struct.data_len = 4;
   buf_struct.seq_num = 0;
   value = 2017952513;
   memcpy(buf_struct.data, &value, sizeof(unsigned int));
 

   //declare a hash out pointer and char array to hold info
   struct hw_packet buf_struct_rcv;
   char hash_out[1024];
   char str[12*1024];
   unsigned int upperByteInd;
	
   printf("sending first hello msg...\n");
   /*Send the buf_struct hello*/
   send(s, &buf_struct, sizeof(struct hw_packet), 0);

   /*Receive server hello*/
   recv(s, &buf_struct_rcv, sizeof(struct hw_packet), 0);
   buf_struct.seq_num = buf_struct_rcv.seq_num;	
   printf("received hello message from the server!\n");


   printf("waiting for the first instruction message...\n");
  /*Receive bufStruct instruct one*/
   recv(s, &buf_struct_rcv, sizeof(struct hw_packet), 0);
   buf_struct.seq_num = buf_struct_rcv.seq_num;

   while(buf_struct_rcv.flag != FLAG_TERMINATE){

   /*Build send response, flag response bit*/
   buf_struct.flag = FLAG_RESPONSE;

   /*Buf struct sequence number gets set to zero*/
   buf_struct.seq_num = 0;

   if(buf_struct_rcv.operation == PUSH){
      printf("received push instruction!!\n");
      printf("received seq_num : %u\n", buf_struct_rcv.seq_num);
      printf("received data_len : %u\n", buf_struct_rcv.data_len);
      upperByteInd = buf_struct_rcv.data_len + buf_struct_rcv.seq_num -1;
      printf("saved bytes from index %u to %u \n", buf_struct_rcv.seq_num, upperByteInd);

      printf("saved byte stream (character representation) : %s\n", buf_struct_rcv.data);
      printf("current file size is : %u !\n\n", upperByteInd+1);

      /*operation type is set to PUSH*/
      buf_struct.operation = PUSH;

      //data length set to zero
      buf_struct.data_len = 0;

      int i;
      for(i = 0; i <= (upperByteInd - buf_struct_rcv.seq_num); i++){
	str[i+buf_struct_rcv.seq_num] = buf_struct_rcv.data[i];
      }
   }
   else if(buf_struct_rcv.operation == DIGEST){
      printf("received digest instruction!!\n");
      printf("********** calculated digest **********\n");

      /*operation type is set to PUSH*/
      buf_struct.operation = DIGEST;

      int len;
      len = strlen(str);

      //call the SHA function to get a 20 byte message to put into the data field
      SHA1(hash_out, str,len);

      printf("***************************************\n");
      printf("sent digest message to server!\n");

      //set the data length to 20
      buf_struct.data_len = 20;

      //set the data field to the 20 byte message digest from SHA-1 algorithm
      strcpy(buf_struct.data, hash_out);
   }

   send(s, &buf_struct, sizeof(struct hw_packet), 0);
   printf("\r\n");

   /*Receive next bufStruct instruct*/
   recv(s, &buf_struct_rcv, sizeof(struct hw_packet), 0);
   }

   printf("received terminate msg! terminating...\n");
   close(s);	
}
