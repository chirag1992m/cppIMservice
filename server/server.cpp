#include "../includes/message.cpp"
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <cstring>
#include <unistd.h>
#include <cerrno>
#include <map>
#include <netdb.h>
#include <arpa/inet.h>
//#include <ifaddrs.h>

using namespace std;

string port = "12345";
int socket_sd;
struct im_message mess;
map<string, struct sockaddr_in> users;

int main() {
	struct addrinfo hints, *servinfo;
	int sockopt_bool=1;
	struct sockaddr_in their_addr;
	socklen_t addr_len = sizeof their_addr;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	
	int rv = getaddrinfo(NULL, port.c_str(), &hints, &servinfo);

	if(rv != 0) {
		printf("getaddrinfo() error: %s\n", gai_strerror(rv));
		return -1;
	}
	
	struct addrinfo *p;
	for(p = servinfo; p != NULL; p = p->ai_next) {
		
		socket_sd = socket( p->ai_family,
							p->ai_socktype,
							p->ai_protocol);
							
		if(socket_sd == -1) {
			printf("socket() error: %s\n", strerror(errno));
			continue;
		}
		
		rv = setsockopt(socket_sd, SOL_SOCKET, SO_REUSEADDR, &sockopt_bool, sizeof(int));
		if(rv == -1) {
			printf("getsockopt() error: %s\n", strerror(errno));
			return false;
		}
		
		rv = bind(socket_sd, p->ai_addr, p->ai_addrlen);
		if(rv == -1) {
			printf("bind() error: %s\n", strerror(errno));
			close(socket_sd);
			continue;
		}
		
		break;
	}
	
	if(p == NULL) {
		printf("addrinfo error: structure empty to set/bind any socket.\n");
		return -1;
	}
	void *buffer[2000];
	
	cout<<"Port Number: "<<port.c_str()<<endl<<endl;
	//cout<<"Server Address: "<<p->ai_addr->sa_data<<endl<<endl;
	freeaddrinfo(servinfo);
	
	//printf("starting to recvfrom\n");
	int numOfBytes;
	
	string username;
	map<string, struct sockaddr_in>::iterator it;
	
	while(true) {
		if((numOfBytes = recvfrom(socket_sd, buffer, 2000, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			printf("recvfrom() error: %s", strerror(errno));
			continue;
		}
		
		memcpy((void *)&mess, buffer, sizeof(struct im_message));
		//cout<<mess.type<<endl<<mess.from<<endl<<mess.to<<endl<<mess.message<<endl;
		username = string(mess.from);
		
		cout<<"message received from "<<username<<endl;
		switch(mess.type) {
			case REGISTRATION_MESSAGE: {
				if(username.compare("IMserver") != 0) {
					if(!users.empty()) {
						it = users.find(username);
						if(it == users.end()) {
							users.insert(pair<string, struct sockaddr_in>(username, their_addr));
							it = users.find(username);
							cout<<"username: "<<username<<", registered with address: "<< it->second.sin_addr.s_addr <<":"<<it->second.sin_port<<endl;
							sprintf(mess.from, "IMserver");
							sprintf(mess.message, "username: %s registered\n", username.c_str());
						} else {
							//send back message that username already exists
							sprintf(mess.from, "IMserver");
							sprintf(mess.message, "username occupied\n");
						}
					} else {
						users.insert(pair<string, struct sockaddr_in>(username, their_addr));
						it = users.find(username);
						char ip[100];
						cout<<"username: "<<username<<", registered with address: "<< inet_ntop(AF_INET, &it->second.sin_addr, ip, sizeof ip) <<":"<<it->second.sin_port<<endl;
						sprintf(mess.from, "IMserver");
						sprintf(mess.message, "username: %s registered\n", username.c_str());
					}
				} else {
					sprintf(mess.from, "IMserver");
					sprintf(mess.message, "username occupied\n");
				}
				if((numOfBytes = sendto(socket_sd, &mess, sizeof(struct im_message), 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
					printf("sendto() error: %s", strerror(errno));
					continue;
				}
				break;
			}
				
			case DEREGISTRATION_MESSAGE: {
				if(!users.empty()) {
					it = users.find(username);
					if(it != users.end()) {
						cout<<"username: "<<username<<", deregistered"<<endl;
						users.erase(it);
					} else
						continue;
				} else
					continue;
				break;
			}
				
			case INSTANT_MESSAGE: {
				if(!users.empty()) {
					username = string(mess.to);
					it = users.find(username);
					if(it != users.end()) {
						if((numOfBytes = sendto(socket_sd, &mess, sizeof(struct im_message), 0, (struct sockaddr *)&it->second, addr_len)) == -1) {
							printf("sendto() error: %s", strerror(errno));
							continue;
						}
					} else {
						
						sprintf(mess.from, "IMserver");
						sprintf(mess.message, "username not found\n");
						if((numOfBytes = sendto(socket_sd, &mess, sizeof(struct im_message), 0, (struct sockaddr *)&their_addr, addr_len)) == -1) {
							printf("sendto() error: %s", strerror(errno));
							continue;
						}
					}
				}
				break;
			}
		}
	}
	
	return 0;
}
