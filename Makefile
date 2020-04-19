CLIENT = warmup_cli
SERVER = warmup_svr

all: $(CLIENT) $(SERVER)

$(CLIENT): $(CLIENT).cpp
	g++ -g -Wall -Werror -o $@ $^

$(SERVER): $(SERVER).cpp
	g++ -g -Wall -Werror -o $@ $^

clean:
	rm -f *.o $(CLIENT) $(SERVER)
