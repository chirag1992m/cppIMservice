#include "../includes/message.cpp"
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <cstring>
#include <unistd.h>
#include <cerrno>

#include <sys/types.h>
#include <sys/time.h>

#include <netdb.h>
#include <arpa/inet.h>

using namespace std;

string host, host_port;
string username;
int client_sd;
void *buffer[2000];
bool registered = false;

void trim(std::string &);

struct im_message mess;

int main(int argc, char *argv[]) {
	if(argc != 4) {
		printf("Invalid argument error\nUsage: ./imc <username> <server name/address> <port_number>\n");
		return -1;
	}
	
	username = string(argv[1]);
	host = string(argv[2]);
	host_port = string(argv[3]);
	
	struct addrinfo hints, *servinfo;
	struct sockaddr_in their_addr;
	socklen_t addr_len = sizeof their_addr;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	
	int rv = getaddrinfo(host.c_str(), host_port.c_str(), &hints, &servinfo);
	if(rv != 0) {
		printf("getaddrinfo() error: %s\n", gai_strerror(rv));
		return false;
	}
	
	struct addrinfo *p;
	for(p = servinfo; p != NULL; p = p->ai_next) {
		
		client_sd = socket( p->ai_family,
							p->ai_socktype,
							p->ai_protocol);
							
		if(client_sd == -1) {
			printf("client socket() error: %s\n", strerror(errno));
			continue;
		}
		
		break;
	}	
	if(p == NULL) {
		printf("client addrinfo error: structure empty to set/connect any socket.\n");
		return false;
	}
	
	mess.type = REGISTRATION_MESSAGE;
	mess.to[0] = '\0';
	strcpy(mess.from, username.c_str());
	mess.message[0] = '\0';
	
	int numOfBytes;
	if((numOfBytes = sendto(client_sd, &mess, sizeof(struct im_message), 0, p->ai_addr, p->ai_addrlen)) == -1) {
		printf("sendto() error: %s", strerror(errno));
		return -1;
	}
	system("clear");
	memset(buffer, 0, 2000);
	//checking the acknowledgement
	recvfrom(client_sd, buffer, 2000, 0, (struct sockaddr *)&their_addr, &addr_len);
	memcpy((void *)&mess, buffer, sizeof(struct im_message));
	cout<<mess.from<<": "<<mess.message<<endl;
	if(!strcmp(mess.message, "username occupied\n") == 0 && strcmp(mess.from, "IMserver") == 0)
		registered = true;
		
	/* multiplexing between the standard input and socket */
	if(registered) {
		fd_set readfds;
		int rval;
		bool cont = true;
		string to, input, messinp;
		size_t pos;
		sprintf((char *)buffer, "imc> ");
		write(1, buffer, 5);
		
		do {
			FD_CLR(client_sd, &readfds);
			FD_CLR(0, &readfds);
			FD_ZERO(&readfds);
			FD_SET(client_sd, &readfds);
			FD_SET(0, &readfds);
			rval = select(client_sd+1, &readfds, NULL, NULL, NULL);
			//cout<<"out of select"<<endl;
			if((rval == 1) && (FD_ISSET(client_sd, &readfds))) {
				//cout<<"message from server!"<<endl;
				/* message from server */
				memset(buffer, 0, 2000);
				recvfrom(client_sd, buffer, 2000, 0, (struct sockaddr *)&their_addr, &addr_len);
				memcpy((void *)&mess, buffer, sizeof(struct im_message));
				
				cout<<mess.from<<": "<<mess.message;
				sprintf((char *)buffer, "imc> ");
				write(1, buffer, 5);
			}
			if((rval == 1) && (FD_ISSET(0, &readfds))) {
				//cout<<"message from stdin!"<<endl;
				/* send message to server from the stdsin */
				memset(buffer, 0, 2000);
				read(0, buffer, 2000);
				input = string((char *)buffer);
				
				//input += string("a");
				//sprintf((char *)buffer, "%s", input.c_str());
				//write(1, buffer, input.length());
				if(input.compare("exit\n") == 0) {
					//sprintf((char *)buffer, "in exit!!!");
					//write(1, buffer, 10);
					break;
				}
				
				pos = input.find(":", 0);
				if(pos == string::npos) {
					sprintf((char *)buffer, "no username found!\nimc> ");
					write(1, buffer, 24);
					continue;
				}
				to = input.substr(0, pos);
				trim(to);
				messinp = input.substr(pos+1, string::npos);
				trim(messinp);
				//cout<<endl<<endl<<to<<endl<<messinp<<endl<<endl;
				mess.type = INSTANT_MESSAGE;
				sprintf(mess.from, "%s", username.c_str());
				sprintf(mess.to, "%s", to.c_str());
				sprintf(mess.message, "%s", messinp.c_str());
				
				memset(buffer, 0, 2000);
				sendto(client_sd, &mess, sizeof(struct im_message), 0, p->ai_addr, p->ai_addrlen);
				sprintf((char *)buffer, "imc> ");
				write(1, buffer, 5);
			}
			if(rval < 0) {
				printf("select() error: %s", strerror(errno));
				cont = false;
			}
		}while(cont);
	}
	/* multiplexing ENDS */
	 
	//deregistration message
	if(registered) {
		mess.type = DEREGISTRATION_MESSAGE;
		mess.to[0] = '\0';
		strcpy(mess.from, username.c_str());
		mess.message[0] = '\0';
		if((numOfBytes = sendto(client_sd, &mess, sizeof(struct im_message), 0, p->ai_addr, p->ai_addrlen)) == -1) {
			printf("sendto() error: %s", strerror(errno));
			return -1;
		}
	}
	
	freeaddrinfo(servinfo);
	close(client_sd);
}

void trim(string &strToTrim) {
	for(size_t i=0; i<strToTrim.length(); ++i) {
		if(strToTrim[i] == ' ' || strToTrim[i] == '\t')
			strToTrim.erase(i, 1);
		else
			break;
	}
	
	for(size_t i=strToTrim.length()-1; i>=0; --i) {
		if(strToTrim[i] == ' ' || strToTrim[i] == '\t')
			strToTrim.erase(i, 1);
		else
			break;
	}
}
