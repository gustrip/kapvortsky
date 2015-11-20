/*
 * microtcp.c
 *
 *  Created on: Oct 25, 2015
 *      Author: surligas
 */

#include "microtcp.h"
#include "../utils/crc32.h"
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


uint32_t seq_number_client,seq_number_server;
struct sockaddr *addr;		//save the sockaddr and the socklen for the shutdown function
socklen_t addr_len;


void error(char *msg) {
  perror(msg);
  exit(-1);
}

microtcp_sock_t
microtcp_socket(int domain, int type, int protocol)
{
  int socket_id;
  microtcp_sock_t s;
  if ((socket_id=socket(domain, type, protocol)) == -1)
    {
	s.state=INVALID;
        return s;
    }
    int optval = 1;
    setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int));
    s.sd=socket_id;
    s.state=UNKNOWN;
    s.init_win_size=0;
    s.curr_win_size=0;
    s.max_win_size=0;
    return s;
  
}

int
microtcp_bind(microtcp_sock_t socket, const struct sockaddr *address,
              socklen_t address_len)
{
     
     
    if( bind(socket.sd , address, address_len) == -1)
    {	
        return -1;
	
    }
    return 1;
}

microtcp_sock_t
microtcp_connect(microtcp_sock_t socket, struct sockaddr *address,
                 socklen_t address_len)
{
	addr=address;
	addr_len=address_len;
	time_t t;
	srand((unsigned) time(&t));
	microtcp_header_t client_header;
	microtcp_header_t * server_header = malloc(sizeof( *server_header));
	seq_number_client=rand()%1000+1;
	client_header.seq_number=seq_number_client;
	client_header.ack_number=0;
	client_header.control=0000000000000010;
	client_header.window=0;
	client_header.data_len=0;
	client_header.future_use0=0;
	client_header.future_use1=0;
	client_header.future_use2=0;
	client_header.checksum=0;
	printf("as a client you are sending an SYN packet\n");
	int k=sendto(socket.sd,(void *)&client_header,sizeof(client_header), 0, address,address_len);
	if (k==-1){
		socket.state=INVALID;
		free(server_header);
		return socket;
	}
	else{	
		if(recvfrom(socket.sd,server_header,sizeof(*server_header), 0, address, &address_len)==-1){
		      socket.state=INVALID;
		      free(server_header);
		      return socket;
		}
		printf("you've received something\n");
		if(server_header->control==0000000000001010 && server_header->ack_number==seq_number_client+1){//recieve SYN+ACK with ack_number N+1
			printf("that something is SYN and ACK from server\n");
			seq_number_server=server_header->seq_number; 
			seq_number_client=seq_number_client+1;
			client_header.seq_number=seq_number_client;
			client_header.ack_number=seq_number_server+1;
			client_header.control=0000000000001000;
			printf("you are sending an ACK to server \n");
			if(sendto(socket.sd,(void *)&client_header,sizeof(client_header), 0, address,address_len)==-1){//send last ACK for the 3-way handshake
				socket.state=INVALID;
				free(server_header);
				return socket;
			}
			printf("establishing connection\n");
			socket.state=ESTABLISHED;
			free(server_header);
			return socket;
		}else {
		      socket.state=INVALID;
		      free(server_header);
		      return socket;
		}
	}
  
  
}

microtcp_sock_t
microtcp_accept(microtcp_sock_t socket, struct sockaddr *address,
                 socklen_t address_len)
{
   addr=address;
   addr_len=address_len;
   time_t t;
   srand((unsigned) time(&t));
   microtcp_header_t server_header;
   microtcp_header_t * header = malloc(sizeof( *header));
   while(1){
     printf("as a server you are waiting for something\n");
    int k=recvfrom(socket.sd,header,sizeof(*header), 0, address,&address_len);
    if (k==-1){
	  socket.state=INVALID;
	  free(header);
	  return socket;
   }else if(k>0){
    printf("you've received something\n");
		if(header->control==0000000000000010){			//if we have a SYN packet
			printf("that something is a SYN packet from server\n");
			seq_number_client=header->seq_number;
			seq_number_server=rand()%1000+1;
			server_header.seq_number=seq_number_server;
			server_header.ack_number=seq_number_client+1;
			server_header.control=0000000000001010;
			server_header.window=0;
			server_header.data_len=0;
			server_header.future_use0=0;
			server_header.future_use1=0;
			server_header.future_use2=0;
			server_header.checksum=0;
			printf("you are sending a SYN and ACK packet to client\n");
			if (sendto(socket.sd,(void *)&server_header,sizeof(server_header),0,address,address_len)==-1){//send SYN && ACK		//send the ack
				socket.state=INVALID;
				free(header);
				return socket;
			}else{
			      
			      if(recvfrom(socket.sd,header,sizeof(*header), 0, address, &address_len)==-1){//receive the final ACK of the 3-way handshake
					socket.state=INVALID;
					free(header);
					return socket;
				
			      }
				printf("you've received something\n");
				if(header->control==0000000000001000 && header->seq_number==seq_number_client+1
					&& header->ack_number==seq_number_server+1)
				{ 
				printf("this something is an ACK packet from client \n");
									    //if seq=previously Seq_client+ and ack is ack_number is seq_server +1 
									    //and ACK packet
					printf("establishing connection\n");
					socket.state=ESTABLISHED;
					free(header);
					return socket;
				}
			}
		}else{
			socket.state=INVALID;
			free(header);
			return socket;
		}
	}
    }
}

microtcp_sock_t
microtcp_shutdown(microtcp_sock_t socket, int how)
{
	if(socket.called_by==0){ //server side
	  
		printf("you are the server\n");
		microtcp_header_t server_header;
		microtcp_header_t * header = malloc(sizeof( *header));
		int k=recvfrom(socket.sd,header,sizeof(*header), 0, addr,&addr_len);
		if (k==-1){
		      socket.state=INVALID;
		      free(header);
		      return socket;
		}else{	printf("you've received something\n");
			if(header->control==0000000000001001){ //FIN && ACK from client
				printf("that something is FIN AND ACK from client\n");
				socket.state=CLOSING_BY_PEER;
				seq_number_client=header->seq_number;
				seq_number_server=seq_number_server+1;
				server_header.seq_number=seq_number_server;
				server_header.ack_number=seq_number_client+1;
				server_header.control=0000000000001000; 		//ACK for the FIN message
				server_header.window=0;
				server_header.data_len=0;
				server_header.future_use0=0;
				server_header.future_use1=0;
				server_header.future_use2=0;
				server_header.checksum=0;
				if (sendto(socket.sd,(void *)&server_header,sizeof(server_header),0,addr,addr_len)==-1){//send  ACK
					socket.state=INVALID;
					free(header);
					return socket;
				}else{
					printf("and you send ACK\n");
					server_header.seq_number=seq_number_server;
					server_header.ack_number=0;
					server_header.control=0000000000001001; //FIN && ACK to close the connection
					if (sendto(socket.sd,(void *)&server_header,sizeof(server_header),0,addr,addr_len)==-1){//send SYN && ACK		//send the ack
						socket.state=INVALID;
						free(header);
						return socket;
					}else{ //waiting to receive last ACK from client
						sleep(1);
						printf("you also send FIN AND ACK \n");
						if (recvfrom(socket.sd,header,sizeof(*header), 0, addr,&addr_len)==-1){
						      socket.state=INVALID;
						      free(header);
						      return socket;
						}else{	
							printf("you've received something\n");
							if(header->control==0000000000001000 && header->seq_number==seq_number_client+1 && 
							  header->ack_number==seq_number_server+1){
							  printf("that something is ACK from client...closing\n");
							  socket.state=CLOSED;
							  return socket;
							}
						}
					  
					  
					}
				}
				
			}else{
				socket.state=INVALID;
				free(header);
				return socket;
			}
		  
		}
	  
	}else{		//client side;
		printf("you are client\n");
		microtcp_header_t client_header;
		microtcp_header_t * server_header = malloc(sizeof( *server_header));
		seq_number_client=seq_number_client+1;
		client_header.seq_number=seq_number_client;
		client_header.ack_number=seq_number_server+1;
		client_header.control=0000000000001001; //FIN AND ACK with ack_number the previously received seq_number_server;
		client_header.window=0;
		client_header.data_len=0;
		client_header.future_use0=0;
		client_header.future_use1=0;
		client_header.future_use2=0;
		client_header.checksum=0;
	  int k=sendto(socket.sd,(void *)&client_header,sizeof(client_header), 0, addr,addr_len);
	  if (k==-1){
			socket.state=INVALID;
			free(server_header);
			return socket;
	  }else{
	    printf("you send a FIN AND ACK packet to server \n");
		if(recvfrom(socket.sd,server_header,sizeof(*server_header), 0, addr, &addr_len)==-1){
		  socket.state=INVALID;
		  free(server_header);
		  return socket;
		}else{
		      printf("you've received something\n");
		      if(server_header->control==0000000000001000 && server_header->ack_number==seq_number_client+1){ //ACK and ack_number=seq_number+1;
				printf("that something is ACK from server\n");
				seq_number_server=server_header->seq_number;
				socket.state=CLOSING_BY_HOST;
				if(recvfrom(socket.sd,server_header,sizeof(*server_header), 0, addr, &addr_len)==-1){
					socket.state=INVALID;
					free(server_header);
					return socket;
				}else{
					printf("you've received something\n");
					if(server_header->control==0000000000001001){ //FIN AND ACK message
						printf("that something is FIN AND ACK from server\n");
						seq_number_client=seq_number_client+1;
						client_header.seq_number=seq_number_client;
						client_header.ack_number=seq_number_server+1;
						client_header.control=0000000000001000; // ACK with ack_number the previously received seq_number_server;
						int k=sendto(socket.sd,(void *)&client_header,sizeof(client_header), 0, addr,addr_len); //last send to server 
						if (k==-1){   
							      socket.state=INVALID;
							      free(server_header);
							      return socket;
						}else{
							printf("sending last ACK to server...closing\n");
							socket.state=CLOSED;
							return socket;
						}
						
						
					}else{			//wrong FIN message
						socket.state=INVALID;
						free(server_header);
						return socket;
					  
					}
				  
				}
			
		      }
		      else{		// not right state (ACK) or not right ack_number
				socket.state=INVALID;
				free(server_header);
				return socket;
			
		      }
		}
	    
	  }
	  
	}
}
