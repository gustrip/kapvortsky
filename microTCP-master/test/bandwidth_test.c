/*
 * bandwidth_test.c
 *
 *  Created on: Oct 25, 2015
 *      Author: surligas
 */
#include "../lib/microtcp.c"
#define CHUNK_SIZE 4096


static inline void
print_statistics(ssize_t received, struct timespec start, struct timespec end)
{
	double elapsed = end.tv_sec - start.tv_sec
		+ (end.tv_nsec - start.tv_nsec) * 1e-9;
	double megabytes = received / (1024.0 * 1024.0);
	printf("Data received: %f MB\n", megabytes);
	printf("Transfer time: %f seconds\n", elapsed);
	printf("Throughput achieved: %f MB/s\n", megabytes / elapsed);
}


int
server_tcp(uint16_t listen_port, const char *file)
{
	uint8_t *buffer;
	FILE *fp;
	int sock;
	int accepted;
	int received;
	ssize_t written;
	ssize_t total_bytes = 0;
	socklen_t client_addr_len;

	struct sockaddr_in sin;
	struct sockaddr client_addr;
	struct timespec start_time;
	struct timespec end_time;

	/* Allocate memory for the application receive buffer */
	buffer = (uint8_t *)malloc(CHUNK_SIZE);
	if(!buffer){
		perror("Allocate application receive buffer");
		return -EXIT_FAILURE;
	}

	/* Open the file for writing the data from the network */
	fp = fopen(file, "w");
	if(!fp){
		perror("Open file for writing");
		free(buffer);
		return -EXIT_FAILURE;
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("Opening TCP socket");
		free(buffer);
		fclose(fp);
		return -EXIT_FAILURE;
	}

	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(listen_port);
	/* Bind to all available network interfaces */
	sin.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) == -1) {
		perror("TCP bind");
		free(buffer);
		fclose(fp);
		return -EXIT_FAILURE;
	}

	if (listen(sock, 1000) == -1) {
		perror("TCP listen");
		free(buffer);
		fclose(fp);
		return -EXIT_FAILURE;
	}

	/* Accept a connection from the client */
	client_addr_len = sizeof(struct sockaddr);
	accepted = accept(sock, &client_addr, &client_addr_len);
	if(accepted < 0){
		perror("TCP accept");
		free(buffer);
		fclose(fp);
		return -EXIT_FAILURE;
	}

	/*
	 * Start processing the received data.
	 *
	 * Also start measuring time. Not the most accurate measurement, but
	 * it is a good starting point.
	 *
	 * At hy-435 we deal with bandwidth measurements software in a more
	 * right and careful way :-)
	 */

	clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
	while( (received = recv(accepted, buffer, CHUNK_SIZE, 0)) > 0){
		written = fwrite(buffer, sizeof(uint8_t), received, fp);
		total_bytes += received;
		if(written * sizeof(uint8_t) != received){
			printf("Failed to write to the file the"
			       " amount of data received from the network.\n");
			shutdown(accepted, SHUT_RDWR);
			shutdown(sock, SHUT_RDWR);
			close(accepted);
			close(sock);
			free(buffer);
			fclose(fp);
			return -EXIT_FAILURE;
		}
	}
	clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
	print_statistics(total_bytes, start_time, end_time);

	shutdown(accepted, SHUT_RDWR);
	shutdown(sock, SHUT_RDWR);
	close(accepted);
	close(sock);
	fclose(fp);
	free(buffer);

	return 0;
}

int
server_microtcp(uint16_t listen_port, const char *file)
{
        FILE *fp;
        ssize_t written;
        ssize_t total_bytes = 0;
        uint8_t *buffer;
        buffer=(uint8_t *)malloc(CHUNK_SIZE);
        int received;
        int port=listen_port;
        struct hostent *hostp;
        char *hostaddrp;
    fp = fopen(file, "w");
	if(!fp){
		perror("Open file for writing");
		free(buffer);
		return -EXIT_FAILURE;
	}
	
        microtcp_sock_t server_st=microtcp_socket(AF_INET, SOCK_DGRAM, 0);
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
        
       // int l=0;
        while( (received=microtcp_recv(&server_st,buffer,CHUNK_SIZE,0)) > 0){
            //l++;
            written = fwrite(buffer, sizeof(uint8_t), received, fp);
            total_bytes += received;
            //printf("received: %d\n",received);
           // printf("written: %d\n",written);
           // printf("total_bytes: %d\n",total_bytes);
           // printf("packet #: %d\n",l);
            if(written * sizeof(uint8_t) != received){
                    printf("Failed to write to the file the"
                            " amount of data received from the network.\n");
                    close(server_st.sd);
                    return -1;
            }
        }
       // l++;
        //printf("number of repeats: %d\n",l);
        if(server_st.state==INVALID){
            error("ERROR at recv when shutdowning");
            return -1;
        }else if(server_st.state==CLOSING_BY_PEER){
        if(server_st.rm_data!=0){
           // printf("data to be written: %d\n",server_st.rm_data);
            written=fwrite(buffer, sizeof(uint8_t),server_st.rm_data, fp);
            //printf(" written: %d\n",written);
        }
        server_st=microtcp_shutdown(server_st,SHUT_RDWR);
        if(server_st.state==INVALID){
            error("ERROR at shutdown");
            return -1;
        }
        close(server_st.sd);
        fclose(fp);
        free(buffer);
        return 0;
        }
        return -1;
}


int
client_tcp(const char *serverip, uint16_t server_port, const char *file)
{
	uint8_t *buffer;
	int sock;
	socklen_t client_addr_len;
	FILE *fp;
	size_t read_items = 0;
	ssize_t data_sent;

	struct sockaddr *client_addr;

	/* Allocate memory for the application receive buffer */
	buffer = (uint8_t *)malloc(CHUNK_SIZE);
	if(!buffer){
		perror("Allocate application receive buffer");
		return -EXIT_FAILURE;
	}

	/* Open the file for writing the data from the network */
	fp = fopen(file, "r");
	if(!fp){
		perror("Open file for reading");
		free(buffer);
		return -EXIT_FAILURE;
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("Opening TCP socket");
		free(buffer);
		fclose(fp);
		return -EXIT_FAILURE;
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family = AF_INET;
	/*Port that server listens at */
	sin.sin_port = htons(server_port);
	/* The server's IP*/
	sin.sin_addr.s_addr = inet_addr(serverip);

	if (connect(sock, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) == -1) {
		perror("TCP connect");
		exit(EXIT_FAILURE);
	}

	printf("Starting sending data...\n");
	/* Start sending the data */
	while( !feof(fp) ){
		read_items = fread(buffer, sizeof(uint8_t), CHUNK_SIZE, fp);
		if(read_items < 1){
			perror("Failed read from file");
			shutdown(sock, SHUT_RDWR);
			close(sock);
			free(buffer);
			fclose(fp);
			return -EXIT_FAILURE;
		}

		data_sent = send(sock, buffer, read_items * sizeof(uint8_t), 0);
		if(data_sent != read_items * sizeof(uint8_t)){
			printf("Failed to send the"
			       " amount of data read from the file.\n");
			shutdown(sock, SHUT_RDWR);
			close(sock);
			free(buffer);
			fclose(fp);
			return -EXIT_FAILURE;
		}

	}

	printf("Data sent. Terminating...\n");
	shutdown(sock, SHUT_RDWR);
	close(sock);
	free(buffer);
	fclose(fp);
	return 0;
}

int
client_microtcp(const char *serverip, uint16_t server_port, const char *file)
{
    size_t read_items = 0;
    ssize_t data_sent;
    FILE *fp;
    fp=fopen(file,"r");
    if(fp==NULL)
    {
        printf("file does not exist\n");
    }

    uint8_t * buffer=(uint8_t *)malloc(CHUNK_SIZE);
    fseek(fp,0,SEEK_SET);
    struct hostent *server;
    char *hostname=serverip;
    int port=server_port;
    
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(-1);
    }
  
    microtcp_sock_t client_st=microtcp_socket(AF_INET, SOCK_DGRAM, 0);
    if(client_st.state==INVALID){
        error("ERROR at creating microtcp_socket"); 
    }

    struct sockaddr_in client_addr,server_addr;
    bzero((char *) &client_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_aton(hostname, &server_addr.sin_addr)==0) {  //server's Internet address
        fprintf(stderr, "inet_aton() failed\n");
        exit(-1);
        
    }
    server_addr.sin_port = htons((unsigned short)port);


    client_st=microtcp_connect(client_st,(struct sockaddr *) &server_addr,sizeof(server_addr));
    if(client_st.state==INVALID){
        error("ERROR at connect function(problem with 3-way handshake"); 
    }
    //int k=0;
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
                //k++;
                //printf("packet #: %d\n",k);
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
    //printf("number of repeats: %d\n",k);
    //printf("Data sent. Terminating...\n");
    client_st=microtcp_shutdown(client_st,SHUT_RDWR);
    if(client_st.state==INVALID){
        error("ERROR at shutdown");
        return -1;
    } 
    close(client_st.sd);
    free(buffer);
    fclose(fp);
    return 0;
}


int
main(int argc, char **argv)
{
	int 		opt;
	int 		port;
	int		exit_code = 0;
	char 		*filestr = NULL;
	char		*ipstr = NULL;
	uint8_t		is_server = 0;
	uint8_t		use_microtcp = 0;

	/* A very easy way to parse command line arguments */
	while ((opt = getopt(argc, argv, "hsmf:p:a:")) != -1) {
		switch (opt)
			{
		/* If -s is set, program runs on server mode */
		case 's':
			is_server = 1;
			break;
		/* if -m is set the program should use the microTCP implementation */
		case 'm':
			use_microtcp = 1;
			break;
		case 'f':
			filestr = strdup(optarg);
			/* A few checks will be nice here...*/
			/* Convert the given file to absolute path */
			break;
		case 'p':
			port = atoi(optarg);
			/* To check or not to check? */
			break;
		case 'a':
			ipstr = strdup(optarg);
			break;

		default:
			printf("Usage: bandwidth_test [-s] [-m] -p port -f file"
			       "Options:\n"
			       "   -s                  If set, the program runs as server. Otherwise as client.\n"
			       "   -m                  If set, the program uses the microTCP implementation. Otherwise the normal TCP.\n"
			       "   -f <string>         If -s is set the -f option specifies the filename of the file that will be saved.\n"
			       "                       If not, is the source file at the client side that will be sent to the server.\n"
			       "   -p <int>            The listening port of the server\n"
			       "   -a <string>         The IP address of the server. This option is ignored if the tool runs in server mode.\n"
			       "   -h                  prints this help\n");
			exit(EXIT_FAILURE);
			}
	}

	/*
	 * TODO: Some error checking here???
	 */

	/*
	 * Depending the use arguments execute the appropriate functions
	 */
	if(is_server){

		if(use_microtcp){
                        printf("mpike edw");
			exit_code = server_microtcp(port, filestr);
		}
		else{
			exit_code = server_tcp(port, filestr);
		}
	}
	else{
		if(use_microtcp){
			exit_code = client_microtcp(ipstr, port, filestr);
		}
		else{
			exit_code = client_tcp(ipstr, port, filestr);
		}
	}

	free(filestr);
	free(ipstr);
	return exit_code;
}

