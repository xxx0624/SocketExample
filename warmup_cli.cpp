#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <typeinfo>
#include <sstream>
using namespace std;


const char DELIMITER = '/';
const int SIZE = 1 << 17;
const int BUFFERSIZE = 100;


int main(int argc, char* argv[]){
    int port, error, sockFD;
    char* temp;
    struct addrinfo hints, *addrs, *it;
    
    if(argc < 3){
        cerr << "Usage ./warmup_cli addr/ip port_number" << endl;
        return EXIT_FAILURE;
    }

    port = strtol(argv[2], &temp, 10);
    if(*temp != '\0'){
        cerr << "port number isn't base of 10" << endl;
        return EXIT_FAILURE;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if((error = getaddrinfo(argv[1], to_string(port).c_str(), &hints, &addrs)) != 0){
        cerr << "fail to get addr[" << argv[1] << "] info, " << gai_strerror(error) << endl;
        return EXIT_FAILURE;
    }
    
    for(it = addrs; it != NULL; it = it->ai_next){
        if((sockFD = socket(it->ai_family, it->ai_socktype, it->ai_protocol)) == -1){
            cout << "fail to create socket, " << gai_strerror(sockFD) << endl;
            continue;
        }
        if((error = connect(sockFD, it->ai_addr, it->ai_addrlen)) == -1){
            close(sockFD);
            cout << "fail to connect, " << gai_strerror(error) << endl;
            continue;
        }
        //successfully
        break;
    }

    if(it == NULL){
        cout << "fail to connect to the server[" << argv[1] << "]" << endl;
        return EXIT_FAILURE;
    }

    cout << "connect to the server[" << argv[1] << "] successful" << endl;
    while(true){
        cout << "Enter a message: ";
        string input;
        getline(cin, input);
        int len = input.length();
        if(len == 0){
            continue;
        }
        if(len > SIZE){
            cerr << "the input size is larger than " << SIZE << endl;
            continue;
        }
        string msg = to_string(len) + DELIMITER + input;

        int nbytes_total = 0;
        while(nbytes_total < (int)msg.length()){
            int nbytes_last = send(sockFD, msg.c_str(), msg.length(), 0);
            if(nbytes_total == -1){
                cerr << "fail to send [" << msg << "] to server, " << gai_strerror(nbytes_total) << endl;
                return EXIT_FAILURE;
            }
            nbytes_total += nbytes_last;
        }

        //error = send(sockFD, msg.c_str(), msg.length(), 0);
        //if(error < 0){
        //    cerr << "fail to send [" << msg << "] to server, " << gai_strerror(error) << endl;
        //} else {
        char buffer[BUFFERSIZE];
        int bufferSize, msgSize = -1, aleadyTakenIn = 0, bufferIdx = 0;
        stringstream msgStream, msgSizeStream;
        bool conversationDone = false;
        while(true){
            bufferIdx = 0;
            bufferSize = recv(sockFD, buffer, BUFFERSIZE, 0);
            if(bufferSize <= 0){
                cerr << "couldn't recv msg from server" << endl;
                break ;
            }

            while(bufferIdx < bufferSize){
                if(msgSize == -1){
                    if(buffer[bufferIdx] == DELIMITER) {
                        char* temp;
                        msgSize = strtol(msgSizeStream.str().c_str(), &temp, 10);
                        if(*temp != '\0'){
                            cerr << "wrong protocol format" << endl;
                            conversationDone = true;
                            break;
                        }
                    } else {
                        msgSizeStream << buffer[bufferIdx];
                    }
                } else {
                    msgStream << buffer[bufferIdx];
                    aleadyTakenIn ++;
                    if(aleadyTakenIn == msgSize){
                        string msg = msgStream.str();
                        cout << msg << endl;
                        conversationDone = true;
                        break;
                    }
                }
                bufferIdx ++;
            }
            if(conversationDone){
                break;
            }
        }
    }
    //}
    close(sockFD);
    freeaddrinfo(addrs);
    return 0;
}