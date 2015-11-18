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
	uint32_t seq_number_client,seq_number_server;
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
	  if(server_header->control==0000000000001010 && server_header->ack_number==seq_number_client+1){//recieve SYN+ACK with ack_number N+1
		seq_number_server=server_header->seq_number;
		seq_number_client=seq_number_client+1;
		client_header.seq_number=seq_number_client;
		client_header.ack_number=seq_number_server+1;
		client_header.control=0000000000001000;
		if(sendto(socket.sd,(void *)&client_header,sizeof(client_header), 0, address,address_len)==-1){//send last ACK for the 3-way handshake
			socket.state=INVALID;
			free(server_header);
			return socket;
		}
		socket.state=ESTABLISHED;
		free(server_header);
		return socket;
	  }
	}
  
  
}


microtcp_sock_t
microtcp_accept(microtcp_sock_t socket, struct sockaddr *address,
                 socklen_t address_len)
{
   uint32_t seq_number_client,seq_number_server;
   time_t t;
   srand((unsigned) time(&t));
   microtcp_header_t server_header;
   microtcp_header_t * header = malloc(sizeof( *header));
   while(1){
    int k=recvfrom(socket.sd,header,sizeof(*header), 0, address,&address_len);
    if (k==-1){
	  socket.state=INVALID;
	  free(header);
	  return socket;
   }else if(k>0){
	if(header->control==0000000000000010){			//if we have a SYN packet
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

			if(header->control==0000000000001000 && header->seq_number==seq_number_client+1
				&& header->ack_number==seq_number_server+1)
			{ 
									    //if seq=previously Seq_client+ and ack is ack_number is seq_server +1 
									    //and ACK packet
				socket.state=ESTABLISHED;
				free(header);
				return socket;
			}
		}
	}

   }
   
   
   }
}

microtcp_sock_t
microtcp_shutdown(microtcp_sock_t socket, int how)
{
	/* Your code here */
}
