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

#define OP_ECHO 		((unsigned char)(0x00))
#define OP_INCREMENT 		((unsigned char)(0x01))
#define OP_DECREMENT 		((unsigned char)(0x02))

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
   buf_struct.operation = OP_ECHO;
   buf_struct.data_len = 4;
   buf_struct.seq_num = 0;
   value = 2017952513;
   memcpy(buf_struct.data, &value, sizeof(unsigned int));
 
   struct hw_packet buf_struct_rcv;

   printf("*** starting ***\r\n\n");
	
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

   printf("received instruction message! received data_len: %u bytes \n", buf_struct_rcv.data_len); 

 memcpy(&value, buf_struct_rcv.data, sizeof(unsigned int));   

   /*Build send response*/
   buf_struct.flag = FLAG_RESPONSE;
   buf_struct.data_len = 4;
   buf_struct.operation = OP_ECHO;

   if(buf_struct_rcv.operation == OP_ECHO){
      printf("operation type is echo.\n");
      printf("echo: %s\n", buf_struct_rcv.data);
   }
   else if(buf_struct_rcv.operation == OP_INCREMENT){
      printf("operation type is increment.\n");
      value++; 
      printf("increment: %u\n", value);
   }
   else if(buf_struct_rcv.operation == OP_DECREMENT){
      printf("operation type is decrement.\n");
      value--; 
      printf("decrement: %u\n", value);
   }
   memcpy(buf_struct.data, &value, sizeof(unsigned int));
   send(s, &buf_struct, sizeof(struct hw_packet), 0);

   printf("sent response msg with seq.num. %u to server\n", buf_struct.seq_num);
   printf("\r\n");

   /*Receive next bufStruct instruct*/
   recv(s, &buf_struct_rcv, sizeof(struct hw_packet), 0);
   buf_struct.seq_num = buf_struct_rcv.seq_num;
   }

   printf("received terminate msg! terminating...\n");
   close(s);	
}
