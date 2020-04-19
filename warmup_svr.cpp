#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
using namespace std;


const int MAX_CONNS = 10;
const int BUFFERSIZE = 100;
const char DELIMITER = '/'; // used to seperate the msg size and msg body


void solve(int cli_sockFD){
    char buffer[BUFFERSIZE];
    int bufferSize, realMsgSize = -1, aleadyTakenIn = 0, bufferIdx = 0;
    stringstream realMsgStream, realMsgSizeStream;
    while(true){
        bufferIdx = 0;
        bufferSize = recv(cli_sockFD, buffer, BUFFERSIZE, 0);
        if(bufferSize <= 0){
            cerr << "couldn't recv msg" << endl;
            return ;
        }

        while(bufferIdx < bufferSize){
            if(realMsgSize == -1){
                if(buffer[bufferIdx] == DELIMITER) {
                    char* temp;
                    realMsgSize = strtol(realMsgSizeStream.str().c_str(), &temp, 10);
                    if(*temp){
                        cerr << "wrong protocol format" << endl;
                        return ;
                    }
                } else {
                    realMsgSizeStream << buffer[bufferIdx];
                }
            } else {
                realMsgStream << buffer[bufferIdx];
                aleadyTakenIn ++;
                if(aleadyTakenIn == realMsgSize){
                    string msg = realMsgStream.str();
                    int len = msg.length();
                    string newMsg = to_string(len) + DELIMITER + msg;
                    cout << "Got: " << newMsg << endl;
                    int error = send(cli_sockFD, newMsg.c_str(), newMsg.length(), 0);
                    if(error < 0){
                        cerr << "fail to send [" << newMsg << "]" << endl;
                    }
                    return ;
                }
            }
            bufferIdx ++;
        }
    }
}


int main(int argc, char *argv[]){
    int port, sockFD, error, yes = 1;
    char* temp;
    struct addrinfo hints, *addrs, *it;

    if(argc != 2){
        cerr << "Usage ./warmup_svr port_number" << endl;
        return EXIT_FAILURE;
    }

    port = strtol(argv[1], &temp, 10);
    if(temp == argv[1]){
        cerr << "port number isn't base of 10" << endl;
        return EXIT_FAILURE;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
    if((error = getaddrinfo(NULL, to_string(port).c_str(), &hints, &addrs)) != 0){
        cerr << "get addr info error, " << gai_strerror(error) << endl;
        return EXIT_FAILURE;
    }

    for(it = addrs; it != NULL; it = it->ai_next){
        if((sockFD = socket(it->ai_family, it->ai_socktype, it->ai_protocol)) == -1){
            cout << "fail to create socket, " << gai_strerror(sockFD) << endl;
            continue;
        }

        if(setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            cout << "set socketopt failed, " << gai_strerror(sockFD) << endl;
            continue; 
        }
        if((error = bind(sockFD, it->ai_addr, it->ai_addrlen)) == -1){
            close(sockFD);
            cout << "fail to connect, " << gai_strerror(error) << endl;
            continue;
        }
        //successfully
        break;
    }
    freeaddrinfo(addrs);

    if(it == NULL){
        cout << "fail to connect to the server[" << argv[1] << "]" << endl;
        return EXIT_FAILURE;
    }

    error = listen(sockFD, MAX_CONNS);
    if(error < 0){
        cerr << "fail to listen the port: " << port << endl;
        return EXIT_FAILURE;
    }

    // keep listening
    while(true){
        struct sockaddr_in cli_addr;
        unsigned int sizelen = sizeof(cli_addr);
        int cli_sockFD = accept(sockFD, (struct sockaddr*)&cli_addr, &sizelen);
        if(cli_sockFD < 0){
            cerr << "Fail to accept the client" << endl;
            //TODO: send error msg back to client
            continue;
        }
        solve(cli_sockFD);
        close(cli_sockFD);
    }
}