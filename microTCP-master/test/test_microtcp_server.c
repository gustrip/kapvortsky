#include "../lib/microtcp.c"
#define CHUNK_SIZE 4096

int main(int argc, char **argv){
    FILE *fp;
    ssize_t written;
    ssize_t total_bytes = 0;
    //char *buffer[3500];
    uint8_t *buffer;
    buffer=(uint8_t *)malloc(CHUNK_SIZE);
    int received;
    
    fp = fopen("temp.jpg", "w");
	if(!fp){
		perror("Open file for writing");
		free(buffer);
		return -EXIT_FAILURE;
	}
    
   
   int port;
   struct hostent *hostp;
   char *hostaddrp;
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  port = atoi(argv[1]);
   
  microtcp_sock_t server_st=microtcp_socket(AF_INET, SOCK_DGRAM, 0);
  //server_st.called_by=00000000;
  if(server_st.state==INVALID){
    error("ERROR at creating microtcp_socket"); 
  }
  
  struct sockaddr_in server_addr,client_addr;
  bzero((char *) &server_addr, sizeof(server_addr)); //server's Internet address
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons((unsigned short)port);

  int k=microtcp_bind(server_st,(struct sockaddr *) &server_addr,sizeof(server_addr));
  if(k<0){
      error("ERROR at binding");
  }
  server_st=microtcp_accept(server_st,(struct sockaddr *) &client_addr,sizeof(client_addr));
  if(server_st.state==INVALID){
    error("ERROR at accept(problem with 3-way handshake"); 
  }
  //find host by address 
  hostp = gethostbyaddr((const char *)&client_addr.sin_addr.s_addr,  
			  sizeof(client_addr.sin_addr.s_addr), AF_INET); 
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(client_addr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
	   hostp->h_name, hostaddrp);
  
   while( (received=microtcp_recv(&server_st,buffer,CHUNK_SIZE,0)) > 0){
            written = fwrite(buffer, sizeof(uint8_t), received, fp);
            total_bytes += received;
            printf("received: %d\n",received);
            if(written * sizeof(uint8_t) != received){
                    printf("Failed to write to the file the"
                            " amount of data received from the network.\n");
                    close(server_st.sd);
                    return -1;
            }
             if(server_st.state==INVALID){
                error("ERROR at recv");
                close(server_st.sd);
                return -1;
            }
   }
    if(server_st.state==INVALID){
            error("ERROR at recv when shutdowning");
    }else if(server_st.state==CLOSING_BY_PEER){
        server_st=microtcp_shutdown(server_st,SHUT_RDWR);
        if(server_st.state==INVALID){
            error("ERROR at shutdown");
        }
        close(server_st.sd);
        fclose(fp);
        free(buffer);
        return 0;
    }
   
    
    
//    if(microtcp_recv(&server_st,buffer,sizeof(buffer),0)==-1){
//        if(server_st.state==INVALID){
//             error("ERROR at recv when shutdowning");
//         }else if(server_st.state==CLOSING_BY_PEER){
//             server_st=microtcp_shutdown(server_st,SHUT_RDWR);
//             if(server_st.state==INVALID){
//                 error("ERROR at shutdown");
//             }  
//             close(server_st.sd);
//         } 
//    
//    }
//     if(server_st.state==INVALID){
//         error("ERROR at recv");
//     }
//     printf("%s\n",buffer);
       
   
    

  
 
}