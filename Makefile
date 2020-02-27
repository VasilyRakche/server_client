OBJS_SERVER = server.o
OBJS_CLIENT = client.o
LIBS = -lstdc++

CFLAGS = -c
CC = g++

PROS = client server 

all: $(PROS)

client: $(OBJS_CLIENT)
	$(CC) -o $@ $^ $(LIBS)

server: $(OBJS_SERVER)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -rf $(PROS) $(OBJS_CLIENT) $(OBJS_SERVER)
