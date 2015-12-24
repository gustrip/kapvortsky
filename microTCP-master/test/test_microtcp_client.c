#include "../lib/microtcp.c"
#define CHUNK_SIZE 4096

int main(int argc, char **argv){
    size_t read_items = 0;
    ssize_t data_sent;
  FILE *fp;
  fp=fopen("IMG_20151130_202634.jpg","r");
  if(fp==NULL)
    {
      printf("file does not exist\n");
    }

  uint8_t * buffer=(uint8_t *)malloc(CHUNK_SIZE);
  fseek(fp,0,SEEK_SET);

  struct hostent *server;
  char *hostname;
  int port;
  if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    port = atoi(argv[2]);

    // get the server's DNS entry 
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }
  
  
  microtcp_sock_t client_st=microtcp_socket(AF_INET, SOCK_DGRAM, 0);
  //client_st.called_by=00000001;
  if(client_st.state==INVALID){
    error("ERROR at creating microtcp_socket"); 
  }
  
  struct sockaddr_in client_addr,server_addr;
  bzero((char *) &client_addr, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
    if (inet_aton(hostname, &server_addr.sin_addr)==0) {  //server's Internet address
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
      
    }
   server_addr.sin_port = htons((unsigned short)port);
  
  
  client_st=microtcp_connect(client_st,(struct sockaddr *) &server_addr,sizeof(server_addr));
  if(client_st.state==INVALID){
    error("ERROR at connect function(problem with 3-way handshake"); 
  }
  while( !feof(fp) ){
		read_items = fread(buffer, sizeof(uint8_t), CHUNK_SIZE, fp);
		if(read_items < 1){
			perror("Failed read from file");
			microtcp_shutdown(client_st,SHUT_RDWR);
			close(client_st.sd);
			free(buffer);
			fclose(fp);
			return -1;
		}

		data_sent =microtcp_send(&client_st, buffer, read_items * sizeof(uint8_t), 0);
		if(data_sent != read_items * sizeof(uint8_t)){
			printf("Failed to send the"
			       " amount of data read from the file.\n");
			microtcp_shutdown(client_st,SHUT_RDWR);
			close(client_st.sd);
			free(buffer);
			fclose(fp);
			return -1;
		}

}

    printf("Data sent. Terminating...\n");
    client_st=microtcp_shutdown(client_st,SHUT_RDWR);
    if(client_st.state==INVALID){
        error("ERROR at shutdown");
    } 
    close(client_st.sd);
    free(buffer);
    fclose(fp);
    return 0;

  
  
  
  
  
//   microtcp_send(&client_st,file_buffer,file_size,0);
//   if(client_st.state==INVALID){
//       error("ERROR at send");
//   }
//   client_st=microtcp_shutdown(client_st,SHUT_RDWR);
//   if(client_st.state==INVALID){
//       error("ERROR at shutdown");
//   } 
  
  
  
}