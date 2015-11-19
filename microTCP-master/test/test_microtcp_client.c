#include "../lib/microtcp.c"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h> 
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
void main(){
  
  
  microtcp_sock_t client_st=microtcp_socket(AF_INET, SOCK_DGRAM, 0);
  client_st.called_by=00000001;
  printf("client id: %d\n",client_st.sd);
  
  struct sockaddr_in client_addr,server_addr;
  
  bzero((char *) &client_addr, sizeof(server_addr));
  
  
   server_addr.sin_family = AF_INET;
    if (inet_aton("192.168.1.102", &server_addr.sin_addr)==0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
      
    }
   server_addr.sin_port = htons((unsigned short)8888);
  
  
  client_st=microtcp_connect(client_st,(struct sockaddr *) &server_addr,sizeof(server_addr));
  
  
  client_st=microtcp_shutdown(client_st,SHUT_RDWR);
  
  
  close(client_st.sd);
  
}