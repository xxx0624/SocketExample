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
#include <bits/stdc++.h> 
using namespace std;


const int MAX_CONNS = 10;
const int BUFFERSIZE = 100;
const char DELIMITER = '/'; // used to seperate the msg size and msg body


int send_msg(int sockFD, string msg){
    int nbytes_total = 0;
    const char* request = msg.c_str();
    while(nbytes_total < (int)msg.length()){
        int nbytes_last = send(sockFD, request + nbytes_total, msg.length() - nbytes_total, 0);
        if(nbytes_last == -1){
            cerr << "fail to send msg back" << gai_strerror(nbytes_last) << endl;
            return nbytes_last;
        }
        nbytes_total += nbytes_last;
    }
    return 0;
}


void process(int cli_sockFD){
    char buffer[BUFFERSIZE];
    int bufferSize, msgSize = -1, aleadyTakenIn = 0, bufferIdx = 0;
    stringstream msgStream, msgSizeStream;
    bool conversationDone = false;
    while(true){
        bufferIdx = 0;
        bufferSize = recv(cli_sockFD, buffer, BUFFERSIZE, 0);
        if(bufferSize < 0){
            cerr << "couldn't recv msg, " << gai_strerror(bufferSize) << endl;
            string errorReply = "Couldn't receive msg. Please re-connect the server.";
            send_msg(cli_sockFD, errorReply);
            return ;
        }

        if(bufferSize == 0){
            cout << "the stream socket peer has been shutdown" << endl;
            return ;
        }

        while(bufferIdx < bufferSize){
            if(msgSize == -1){
                if(buffer[bufferIdx] == DELIMITER) {
                    char* temp;
                    msgSize = strtol(msgSizeStream.str().c_str(), &temp, 10);
                    if(*temp != '\0'){
                        cerr << "wrong protocol format. shutdown the connection." << endl;
                        string errorReply = "Wrong protocol format. Please re-connect the server.";
                        send_msg(cli_sockFD, errorReply);
                        return ;
                    }
                } else {
                    msgSizeStream << buffer[bufferIdx];
                }
            } else {
                msgStream << buffer[bufferIdx];
                aleadyTakenIn ++;
                if(aleadyTakenIn == msgSize){
                    string msg = msgStream.str();
                    cout << "Recv msg:" << msg << endl;
                    reverse(msg.begin(), msg.end()); 
                    int len = msg.length();
                    string reply = to_string(len) + DELIMITER + msg;
                    cout << "Send msg back:" << reply << endl;
                    send_msg(cli_sockFD, reply);
                    conversationDone = true;
                    break;
                }
            }
            bufferIdx ++;
        }
        if(conversationDone){
            // start next round conversation
            conversationDone = false;
            msgSize = -1, aleadyTakenIn = 0;
            msgSizeStream.str("");
            msgStream.str("");
        }
    }
}


int main(int argc, char *argv[]){
    int port, sockFD, error;
    char* temp;
    struct sockaddr_in serv_addr;

    if(argc != 2){
        cerr << "Usage ./warmup_svr port_number" << endl;
        return EXIT_FAILURE;
    }

    port = strtol(argv[1], &temp, 10);
    if(*temp != '\0'){
        cerr << "port number isn't base of 10" << endl;
        return EXIT_FAILURE;
    }

    if((sockFD = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cerr << "fail to create socket, " << gai_strerror(sockFD) << endl;
        return EXIT_FAILURE;
    }

    memset(&serv_addr, 0, sizeof serv_addr);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    
    error = bind(sockFD, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if(error < 0){
        close(sockFD);
        cout << "fail to bind, " << gai_strerror(error) << endl;
        return EXIT_FAILURE;
    }

    error = listen(sockFD, MAX_CONNS);
    if(error < 0){
        close(sockFD);
        cerr << "fail to listen the port: " << port << gai_strerror(error) << endl;
        return EXIT_FAILURE;
    }

    // keep listening
    while(true){
        struct sockaddr_in cli_addr;
        unsigned int sizelen = sizeof(cli_addr);
        int cli_sockFD = accept(sockFD, (struct sockaddr*)&cli_addr, &sizelen);
        if(cli_sockFD < 0){
            cerr << "fail to accept the client, " << gai_strerror(error) << endl;
            continue;
        }
        process(cli_sockFD);
        close(cli_sockFD);
    }
}