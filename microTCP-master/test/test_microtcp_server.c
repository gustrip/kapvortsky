#include "../lib/microtcp.c"


void main(){
  
  struct hostent *hostp;
   char *hostaddrp;
  
  microtcp_sock_t server_st=microtcp_socket(AF_INET, SOCK_DGRAM, 0);
   
  printf("server id: %d\n",server_st.sd);
  
  struct sockaddr_in server_addr,client_addr;
  
  bzero((char *) &server_addr, sizeof(server_addr));
  
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons((unsigned short)8888);

  int k=microtcp_bind(server_st,(struct sockaddr *) &server_addr,sizeof(server_addr));
  if(k<0){
      printf("error bind\n");
  }else{
    
   printf("success from bind\n"); 
  }
  
  server_st=microtcp_accept(server_st,(struct sockaddr *) &client_addr,sizeof(client_addr));
  
  hostp = gethostbyaddr((const char *)&client_addr.sin_addr.s_addr, 
			  sizeof(client_addr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(client_addr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
	   hostp->h_name, hostaddrp);
  
 close(server_st.sd);

  
 
}