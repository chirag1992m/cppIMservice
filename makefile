CC=g++
CFLAG=-Wall

ims: imc server/server.cpp
	$(CC) $(CFLAG) server/server.cpp -o ims

imc: client/client.cpp
	$(CC) $(CFLAG) client/client.cpp -o imc
