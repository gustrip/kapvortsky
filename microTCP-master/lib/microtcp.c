/*
 * microtcp.c
 *
 *  Created on: Oct 25, 2015
 *      Author: surligas
 */

#include "microtcp.h"
#include "../utils/crc32.h"



void set_timeout(microtcp_sock_t *socket){
    struct timeval timeout;
    timeout.tv_sec =0;
    timeout.tv_usec = MICROTCP_ACK_TIMEOUT_US ;
    if ( setsockopt (socket->sd,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval)) < 0) 
    {
        perror (" setsockopt ");
    }
}
void print_header(microtcp_header_t *header){
    printf("\nseq_number: %d\n",header->seq_number);
    printf("ack_number: %d\n",header->ack_number);
    printf("control: %d\n",header->control);
    printf("window: %d\n",header->window);
    printf("data_len: %d\n\n",header->data_len);
    
}

uint8_t error_checking(microtcp_header_t *head,void *buf,size_t len){ //error_checking for the whole packet
    uint32_t rcv_csum=head->checksum;
    head->checksum=0;
    uint32_t sum=crc32(buf,len);
    if(rcv_csum==sum){
        return 1;
    }
    return -1;
    
}
uint32_t make_checksum(microtcp_header_t *head,void *buf, size_t len){ //make checksum for a whole packet
    head->checksum=0;
    uint32_t csum=crc32(buf,len);
    return csum;
}

microtcp_header_t create_header1(microtcp_sock_t *socket,size_t length){ //create header with checksum just 0
    microtcp_header_t  header ;
    header.seq_number=socket->seq_number;
    header.ack_number=socket->ack_number;
    header.control=0000000000001000;
    header.window=0;
    header.data_len=length;
    header.future_use0=0;
    header.future_use1=0;
    header.future_use2=0;
    header.checksum=0;
    return header;
    
}


uint8_t error_checking1(microtcp_header_t *buf, size_t len){ //error_checking for header only
    uint32_t rcv_csum=buf->checksum;
    buf->checksum=0;
    uint32_t sum=crc32(buf,len);
    if(rcv_csum==sum){
        return 1;
    }
    return -1;
    
}
uint32_t make_checksum1(microtcp_header_t *buf, size_t len){ //checksum just for header packet
    buf->checksum=0;
    uint32_t csum=crc32(buf,len);
    return csum;
}

microtcp_header_t create_header(microtcp_sock_t *socket,size_t length){ //create header with the right checksum
    microtcp_header_t  header ;
    header.seq_number=socket->seq_number;
    header.ack_number=socket->ack_number;
    header.control=0000000000001000;
    header.window=0;
    header.data_len=length;
    header.future_use0=0;
    header.future_use1=0;
    header.future_use2=0;
    header.checksum=0;
    header.checksum=make_checksum1(&header,sizeof(microtcp_header_t));
    return header;
    
}

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
    s.buf_fill_level=0;
    s.cwnd=0;
    s.ssthresh=0;
    s.seq_number=0;
    s.ack_number=0;
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
	socket.addr=address;
	socket.addr_len=address_len;
	time_t t;
	srand((unsigned) time(&t));
	microtcp_header_t client_header;
	microtcp_header_t * server_header = malloc(sizeof( *server_header));
	socket.seq_number=rand()%1000+1;
	client_header.seq_number=socket.seq_number;
        socket.ack_number=0;
	client_header.ack_number=socket.ack_number;
	client_header.control=0000000000000010;
	client_header.window=0;
	client_header.data_len=0;
	client_header.future_use0=0;
	client_header.future_use1=0;
	client_header.future_use2=0;
	client_header.checksum=0;
        client_header.checksum=make_checksum1(&client_header,sizeof(client_header));
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
                if(!error_checking1(server_header,sizeof(*server_header))){     //bad packet at error checking 
                    printf("bad connection.socket is closing.maybe sender will communicate later\n");
                    socket.state=INVALID;
                    free(server_header);
                    return socket;
                }
		if(server_header->control==0000000000001010 && server_header->ack_number==socket.seq_number+1){//recieve SYN+ACK with ack_number N+1
			printf("that something is SYN and ACK from server\n");
                        socket.init_win_size=server_header->window;
                        socket.curr_win_size=server_header->window;
			socket.ack_number=server_header->seq_number; //auto pou tha prepei na perimenei
			socket.seq_number=server_header->ack_number; //auto pou tha perimenoume na steilei
			client_header.seq_number=socket.seq_number;
			client_header.ack_number=socket.ack_number;
			client_header.control=0000000000001000;
			printf("you are sending an ACK to server \n");
                        client_header.checksum=make_checksum1(&client_header,sizeof(client_header));
			if(sendto(socket.sd,(void *)&client_header,sizeof(client_header), 0, address,address_len)==-1){//send last ACK for the 3-way handshake
				socket.state=INVALID;
				free(server_header);
				return socket;
			}
			printf("establishing connection\n");
                        
                        
			socket.state=ESTABLISHED;
                        socket.recvbuf=(uint8_t *)malloc(MICROTCP_RECVBUF_LEN);
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
   uint32_t csum,rcv_csum;
   socket.addr=address;
   socket.addr_len=address_len;
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
    if(!error_checking1(header,sizeof(*header))){     //bad packet at error checking 
        printf("bad connection.socket is closing.maybe sender will communicate later\n");
        socket.state=INVALID;
        free(header);
        return socket;
    }
    printf("you've received something\n");
		if(header->control==0000000000000010){			//if we have a SYN packet
			printf("that something is a SYN packet from server\n");
			socket.ack_number=header->seq_number;
			socket.seq_number=rand()%1000+1;
                        socket.curr_win_size=MICROTCP_WIN_SIZE;
                        socket.init_win_size=MICROTCP_WIN_SIZE;
			server_header.seq_number=socket.seq_number;
			server_header.ack_number=socket.ack_number+1;
			server_header.control=0000000000001010;
			server_header.window=MICROTCP_WIN_SIZE;
			server_header.data_len=0;
			server_header.future_use0=0;
			server_header.future_use1=0;
			server_header.future_use2=0;
			server_header.checksum=0;
                        server_header.checksum=make_checksum1(&server_header,sizeof(server_header));
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
                                if(!error_checking1(header,sizeof(*header))){     //bad packet at error checking 
                                    printf("bad connection.socket is closing.maybe sender will communicate later\n");
                                    socket.state=INVALID;
                                    free(header);
                                    return socket;
                                }
;
				if(header->control==0000000000001000 && header->seq_number==socket.ack_number+1
					&& header->ack_number==socket.seq_number)
				{ 
				printf("this something is an ACK packet from client \n");
									    //if seq=previously Seq_client+ and ack is ack_number is seq_server +1 
									    //and ACK packet
					printf("establishing connection\n");
					socket.state=ESTABLISHED;
                                        socket.recvbuf=(uint8_t *)malloc(MICROTCP_RECVBUF_LEN);
                                        socket.ack_number+=1;
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
	if(socket.state==CLOSING_BY_PEER){ //server side
                microtcp_header_t * header = malloc(sizeof( *header));
		printf("you are the server\n");
		microtcp_header_t server_header=create_header(&socket,0);
                server_header.seq_number=socket.ack_number;
                server_header.ack_number=1;
                socket.seq_number=1;
                server_header.control=0000000000001000; 		//ACK for the FIN message
                if (sendto(socket.sd,(void *)&server_header,sizeof(server_header),0,socket.addr,socket.addr_len)==-1){//send  ACK
                        socket.state=INVALID;
                        free(header);
                        free(socket.recvbuf);
                        return socket;
                }else{  
                        printf("and you send ACK\n");
                        server_header.seq_number=socket.ack_number+1;
                        server_header.control=0000000000001001; //FIN && ACK to close the connection
                        if (sendto(socket.sd,(void *)&server_header,sizeof(server_header),0,socket.addr,socket.addr_len)==-1){//send SYN && ACK		//send the ack
                                socket.state=INVALID;
                                free(header);
                                free(socket.recvbuf);
                                return socket;
                        }else{ //waiting to receive last ACK from client
                                //sleep(1);
                                printf("you also send FIN AND ACK \n");
                                if (recvfrom(socket.sd,header,sizeof(*header), 0, socket.addr,&socket.addr_len)==-1){
                                        socket.state=INVALID;
                                        free(header);
                                        free(socket.recvbuf);
                                        return socket;
                                }else{	
                                        printf("you've received something\n");
                                        if(header->control==0000000000001000 && header->seq_number==socket.seq_number){
                                            printf("that something is ACK from client...closing\n");
                                            socket.state=CLOSED;
                                        free(socket.recvbuf);
                                            return socket;
                                        }
                                }
                            
                            
                        }
                }
                        
                
            
            
	  
	}else{		//client side;
		printf("you are client\n");
                void *send_buffer;
                socket.seq_number=socket.seq_number+1;
		microtcp_header_t client_header=create_header(&socket,0);
		microtcp_header_t * server_header = malloc(sizeof(microtcp_header_t));
		client_header.control=0000000000001001; //FIN AND ACK with ack_number the previously received seq_number_server;
                send_buffer=malloc(sizeof(microtcp_header_t)); //32 bytes for header and the rest for the buffer;
                memcpy(send_buffer,&client_header,sizeof(microtcp_header_t));
	  int k=sendto(socket.sd,send_buffer,sizeof(microtcp_header_t), 0, socket.addr,socket.addr_len);
	  if (k==-1){
			socket.state=INVALID;
                        free(send_buffer);
			free(server_header);
                        free(socket.recvbuf);
			return socket;
	  }else{
	    printf("you send a FIN AND ACK packet to server \n");
		if(recvfrom(socket.sd,server_header,sizeof(*server_header), 0, socket.addr, &socket.addr_len)==-1){
		  socket.state=INVALID;
		  free(server_header);
                  free(socket.recvbuf);
                  free(send_buffer);
		  return socket;
		}else{
		      printf("you've received something\n");
		      if(server_header->control==0000000000001000 && server_header->seq_number==socket.seq_number){ //ACK and ack_number=seq_number+1;
				printf("that something is ACK from server\n");
				socket.state=CLOSING_BY_HOST;
				if(recvfrom(socket.sd,server_header,sizeof(*server_header), 0, socket.addr, &socket.addr_len)==-1){
					socket.state=INVALID;
					free(server_header);
                                        free(socket.recvbuf);
                                        free(send_buffer);
					return socket;
				}else{
					printf("you've received something\n");
					if(server_header->control==0000000000001001){ //FIN AND ACK message
						printf("that something is FIN AND ACK from server\n");
                                                socket.seq_number=1;
                                                socket.ack_number+=1;
						client_header=create_header(&socket,0);
						client_header.control=0000000000001000; // ACK with ack_number the previously received seq_number_server;
						int k=sendto(socket.sd,(void *)&client_header,sizeof(client_header), 0, socket.addr,socket.addr_len); //last send to server 
						if (k==-1){   
							      socket.state=INVALID;
							      free(server_header);
                                                              free(socket.recvbuf);
							      return socket;
						}else{
							printf("sending last ACK to server...closing\n");
							socket.state=CLOSED;
                                                        free(socket.recvbuf);
                                                        free(send_buffer);
							return socket;
						}
						
						
					}else{			//wrong FIN message
						socket.state=INVALID;
						free(server_header);
                                                free(socket.recvbuf);
                                                free(send_buffer);
						return socket;
					  
					}
				  
				}
			
		      }
		      else{		// not right state (ACK) or not right ack_number
				socket.state=INVALID;
				free(server_header);
                                free(send_buffer);
				return socket;
			
		      }
		}
	    
	  }
	  
	}
}
ssize_t
microtcp_send(microtcp_sock_t *socket, const void *buffer, size_t length, int flags)
{       
        void *send_buffer,*rcv_buffer;
        send_buffer=malloc(MICROTCP_MSS); //32 bytes for header and the rest for the buffer;
        rcv_buffer=malloc(sizeof(microtcp_header_t)); //only the header;
        set_timeout(socket);
        int remaining;
        int data_sent=0;
        int bytes_to_send,chunks;
        remaining=length;
        Retransmissions:
        while(data_sent<length){
//              printf("data_send: %d-----",data_sent);
//              printf("remaining: %d-----",remaining);
            bytes_to_send=remaining;
//              printf("bytes_to_send: %d----",bytes_to_send);
            chunks=bytes_to_send/MICROTCP_MSS;
            int i;
            for(i = 0; i < chunks ; i++){
                
                buffer+=i*(MICROTCP_MSS-sizeof(microtcp_header_t));
                socket->ack_number+=sizeof(microtcp_header_t);
                microtcp_header_t  header =create_header1(socket,length);
                header.data_len=MICROTCP_MSS-sizeof(microtcp_header_t);
                header.window=socket->curr_win_size;
                memcpy(send_buffer,&header,sizeof(microtcp_header_t));
                send_buffer+=sizeof(microtcp_header_t);
                memcpy(send_buffer,buffer,MICROTCP_MSS-sizeof(microtcp_header_t));
                send_buffer-=sizeof(microtcp_header_t);
                uint32_t csum=make_checksum(&header,send_buffer,MICROTCP_MSS);       //create checksum and assign it to the right header option
                header.checksum=csum;
                memcpy(send_buffer,&header,sizeof(microtcp_header_t));
                int k=sendto(socket->sd,send_buffer,+MICROTCP_MSS, 0, socket->addr,socket->addr_len);                
                socket->seq_number+=k;
                
            }
            int s=bytes_to_send % MICROTCP_MSS;
            if(s){
                memset(send_buffer,0,MICROTCP_MSS);
                chunks++;
                buffer+=i*(MICROTCP_MSS-sizeof(microtcp_header_t));
                socket->ack_number+=sizeof(microtcp_header_t); //auto pou perimenw na erthei pisw
                microtcp_header_t  header =create_header1(socket,length);
                header.data_len=length-(i*(MICROTCP_MSS-sizeof(microtcp_header_t)));
                header.window=socket->curr_win_size;
                memcpy(send_buffer,&header,sizeof(microtcp_header_t));
                send_buffer+=sizeof(microtcp_header_t);
                memcpy(send_buffer,buffer,length-(i*(MICROTCP_MSS-sizeof(microtcp_header_t))));
                send_buffer-=sizeof(microtcp_header_t);
                uint32_t csum=make_checksum(&header,send_buffer,(length-(i*(MICROTCP_MSS-sizeof(microtcp_header_t)))));       //create checksum and assign it to the right header option
                header.checksum=csum;
                memcpy(send_buffer,&header,sizeof(microtcp_header_t));
                int k=sendto(socket->sd,send_buffer,(length-(i*(MICROTCP_MSS-sizeof(microtcp_header_t)))), 0, socket->addr,socket->addr_len);
                socket->seq_number+=k;
                
                
            }
            buffer-=chunks*(MICROTCP_MSS-sizeof(microtcp_header_t)); //init buffer in case of retransmission
            
            for(int i = 0; i < chunks ; i++){
                int k=recvfrom(socket->sd,rcv_buffer,sizeof(microtcp_header_t), 0, socket->addr,&socket->addr_len);
                if (k==-1){
                      printf("timeout \n");
                      //int data_sent=0;
                      //remaining=length;
                      //goto Retransmissions;
                }
                microtcp_header_t *header=malloc(sizeof(microtcp_header_t));
                memcpy(header,rcv_buffer,sizeof(microtcp_header_t));
                if(!error_checking1(header,sizeof(microtcp_header_t))){
                    printf("#############error###############\n");
                    goto Retransmissions;
                }
                uint32_t ack=socket->ack_number-(chunks-i)*sizeof(microtcp_header_t);
                if(header->seq_number==ack){
                   // printf("right ack is right\n");
                }else{
                     printf("wrong ack received\n");
                }
                
//                memcpy(socket->recvbuf,header,sizeof(microtcp_header_t));
            }
            
//              printf("chunks: %d\n",chunks);
            remaining -= bytes_to_send ;
            data_sent += bytes_to_send ;
//              printf("data_send: %d\n",data_sent);
//              printf("remaining: %d\n",remaining);
        }
        buffer-=data_sent;
        
        //retransmission pending
        
        free(send_buffer);
	return data_sent;
        
}

ssize_t
microtcp_recv(microtcp_sock_t *socket, void *buffer, size_t length, int flags)
{   
    set_timeout(socket); //setting timeout to MICROTCP_ACK_TIMEOUT_US
    void *rcv_buffer;
    while(1){
        rcv_buffer=malloc(MICROTCP_MSS);
        microtcp_header_t * header = malloc(sizeof( microtcp_header_t ));
        int k=recvfrom(socket->sd,rcv_buffer,MICROTCP_MSS, 0, socket->addr,&socket->addr_len);
        if (k==-1){
                printf("#########timeout#######\n");
                microtcp_header_t sending_header=create_header(socket,0);
                sending_header.window=MICROTCP_RECVBUF_LEN-socket->buf_fill_level;
                sending_header.data_len=socket->buf_fill_level;
                int m=sendto(socket->sd,&sending_header,sizeof(microtcp_header_t), 0, socket->addr,socket->addr_len);
        }else{	printf("you've received something\n");
                memcpy(header,rcv_buffer,sizeof(microtcp_header_t));
                if(!error_checking(header,rcv_buffer,MICROTCP_MSS)){ //error_checking
                    printf("#############error###############\n");
                    k=-1;
                }
                rcv_buffer+=sizeof(microtcp_header_t);          //move pointer to the data
                if(header->control==0000000000001001 ) { //FIN && ACK from client
                        printf("that something is FIN AND ACK from client\n");
                        socket->state=CLOSING_BY_PEER;
                        socket->ack_number=header->seq_number;
                        memcpy(buffer,socket->recvbuf,socket->buf_fill_level);
                        socket->rm_data=socket->buf_fill_level;
                        return -1;
                }
                else if(header->seq_number==socket->ack_number){
                        printf("%%%%%%%%success%%%%%%%\n");
                        if(MICROTCP_RECVBUF_LEN>=length){
                            if(MICROTCP_MSS-sizeof(microtcp_header_t)<length){ //if the external buffer of the application is less than recvbuf of the socket 
                            if(socket->buf_fill_level+MICROTCP_MSS-sizeof(microtcp_header_t)>length){
                                memcpy(buffer,socket->recvbuf,socket->buf_fill_level);
                                memcpy(socket->recvbuf,rcv_buffer,header->data_len);
                                size_t temp=socket->buf_fill_level;
                                socket->buf_fill_level=0;
                                socket->buf_fill_level+=header->data_len;
                                printf("ta data pou girname: %d\n",temp);
                                socket->ack_number+=k;
                                microtcp_header_t sending_header=create_header(socket,0);
                                sending_header.window=MICROTCP_RECVBUF_LEN-socket->buf_fill_level;
                                sending_header.data_len=sizeof(microtcp_header_t);
                                int m=sendto(socket->sd,&sending_header,sizeof(microtcp_header_t), 0, socket->addr,socket->addr_len);
                                socket->seq_number+=m;
                                return temp;
                            }
                            socket->recvbuf+=socket->buf_fill_level;    //change tha pointer of recvbuf 
                            memcpy(socket->recvbuf,rcv_buffer,header->data_len); //copy rcv_buffer of send to recvbuf of socket
                            socket->recvbuf-=socket->buf_fill_level;
                            socket->buf_fill_level+=header->data_len;       // change the buf_fill_level to new size
                            
                        }else if(MICROTCP_MSS-sizeof(microtcp_header_t)>length){
                            memcpy(buffer,rcv_buffer,length);
                            return length;
                            
                        }
                            
                            
                        }else{
                            
                            
                        }
                        
                        
                        
                }else{
                    printf("packet loss:wrong seq_number received\n");
                    microtcp_header_t sending_header=create_header(socket,0);
                    sending_header.window=MICROTCP_RECVBUF_LEN-socket->buf_fill_level;
                    sending_header.data_len=socket->buf_fill_level;
                    int m=sendto(socket->sd,&sending_header,sizeof(microtcp_header_t), 0, socket->addr,socket->addr_len);
                }
        }
        socket->ack_number+=k;
        microtcp_header_t sending_header=create_header(socket,0);
        sending_header.window=MICROTCP_RECVBUF_LEN-socket->buf_fill_level;
        sending_header.data_len=sizeof(microtcp_header_t);
        int m=sendto(socket->sd,&sending_header,sizeof(microtcp_header_t), 0, socket->addr,socket->addr_len);
        socket->seq_number+=m;
    }
}


