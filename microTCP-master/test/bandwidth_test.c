/*
 * bandwidth_test.c
 *
 *  Created on: Oct 25, 2015
 *      Author: surligas
 */
#include "../lib/microtcp.c"

int
server_tcp(uint16_t listen_port, char *file)
{
	int sock,nsocket; 
	int addr_len; 
	struct sockaddr_in addr_server;
	struct sockaddr_in addr_client;
	char recvbuf[1024]; 
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1 )
	{
	    error("ERROR: Failed to obtain Socket Descriptor.\n");
	}
	else{ 
	    printf("[Server] Obtaining socket descriptor successfully.\n");
	}

	addr_server.sin_family = AF_INET; 
	addr_server.sin_port = htons(listen_port); 
	addr_server.sin_addr.s_addr = INADDR_ANY; 
	bzero(&(addr_server.sin_zero), 8); 

	/* Bind a special Port */
	if( bind(sock, (struct sockaddr*)&addr_server, sizeof(struct sockaddr)) == -1 )
	{
	    error("ERROR: Failed to bind Port.\n");
	}
	else{ 
		printf("[Server] Binded tcp port %d in addr 127.0.0.1 sucessfully.\n",listen_port);
	}
	if(listen(sock,0) == -1)
	{
	    error("ERROR: Failed to listen Port.\n");
	}
	else{
		printf ("[Server] Listening the port %d successfully.\n", listen_port);
	}

	addr_len = sizeof(struct sockaddr_in);

    /* Wait a connection, and obtain a new socket file despriptor for single connection */
    if ((nsocket = accept(sock, (struct sockaddr *)&addr_client, &addr_len)) == -1) 
	{
	error("ERROR: Obtaining new Socket Despcritor.\n");
	}
    else {
		printf("[Server] Server has got connected from %s.\n", inet_ntoa(addr_client.sin_addr));
    }
    char* fl = file;
    FILE *fp = fopen(fl, "a");
    if(fp == NULL)
	    printf("File %s Cannot be opened file on server.\n", fl);
    else
    {
	    bzero(recvbuf, 1024); 
	    int k = 0;
	    
	    while((k = recv(nsocket, recvbuf, 1024, 0))>=0) 
	    {	
		//printf("bytes:%d\n",k);
		if (k == 0) 
				{
		      break;
				}
		
		int m = fwrite(recvbuf, sizeof(char), k, fp);
		if(m < k)
		{
		    error("File write failed on server.\n");
		}
		if(k < 0)
		{
		    error("Error receiving file from client to server.\n");
		}
		bzero(recvbuf, 1024);
	    }
	    printf("Ok received from client!\n");
	    fclose(fp);
   }
	
	close(sock);
	return 0;
}

int
server_microtcp(uint16_t listen_port, char *file)
{
	/*TODO: Write your code here */
	return 0;
}


int
client_tcp(uint16_t server_port, char *file)
{
	int sock,nsocket;
	char sbuffer[1024];
	struct sockaddr_in server_addr;
	if((sock=socket(AF_INET,SOCK_STREAM,0))==-1){
	   error("ERROR: Failed to obtain Socket Descriptor!\n");
	}
	server_addr.sin_family = AF_INET; 
	server_addr.sin_port = htons(server_port); 
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr); 
	bzero(&(server_addr.sin_zero), 8);
	
	
	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
	{
	    error("ERROR: Failed to connect to the host!\n");
	}
	
	char* fl = file;
	printf("[Client] Sending %s to the Server...", fl);
	FILE *fp = fopen(fl, "r");
	if(fp == NULL)
	{
	    printf("ERROR: File %s not found.\n", fl);
		exit(1);
	}
	
	
	bzero(sbuffer, 1024); 
	int k; 
	while((k= fread(sbuffer, sizeof(char), 1024, fp))>0){
	  int m=send(sock, sbuffer, k, 0);
		if( k< 0)
		{
		    printf("ERROR: Failed to send file %s.\n", fl);
		    break;
		}
		//printf("bytes readed:%d\n",k);
		//printf("bytes sent:%d\n",m);
		bzero(sbuffer, 1024);
	}
	printf("Ok File %s from Client was Sent!\n", fl);
	fclose (fp);
	close(sock);
	printf("[Client] Connection lost.\n");
	
	return 0;
}

int
client_microtcp(uint16_t server_port, char *file)
{
	/*TODO: Write your code here */
	return 0;
}


int
main(int argc, char **argv)
{
	int 		opt;
	int 		port;
	int		exit_code = 0;
	char 		*filestr;
	uint8_t		is_server = 0;
	uint8_t		use_microtcp = 0;

	/* A very easy way to parse command line arguments */
	while ((opt = getopt(argc, argv, "hsmf:p:")) != -1) {
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
			filestr = realpath(filestr, NULL);
			break;
		case 'p':
			port = atoi(optarg);
			/* To check or not to check? */
			break;

		default:
			printf("Usage: bandwidth_test [-s] [-m] -p port -f file"
			       "Options:\n"
			       "   -s                  If set, the program runs as server. Otherwise as client.\n"
			       "   -m                  If set, the program uses the microTCP implementation. Otherwise the normal TCP.\n"
			       "   -f <string>         If -s is set the -f option specifies the filename of the file that will be saved.\n"
			       "                       If not, is the source file at the client side that will be sent to the server.\n"
			       "   -p <int>            The listening port of the server\n"
			       "   -h                  prints this help\n");
			exit(EXIT_FAILURE);
			}
	}

	/*
	 * TODO: Some error cheking here???
	 */

	/*
	 * Depending the use arguements execute the appropriate functions
	 */
	if(is_server){
		if(use_microtcp){
			exit_code = server_microtcp(port, filestr);
		}
		else{
			exit_code = server_tcp(port, filestr);
			free(filestr);
		}
	}
	else{
		if(use_microtcp){
			exit_code = client_microtcp(port, filestr);
		}
		else{
			exit_code = client_tcp(port, filestr);
		}
	}
	return exit_code;
}

