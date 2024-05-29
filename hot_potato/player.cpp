#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <stdio.h>
#include <string>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h> 
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <netinet/in.h>

#include"potato.h"

using namespace std;

class Player{
public:
    int master_socket_fd;
    int id;
    int players;
    int listen_socket_fd;
    int next_fd;
    int prev_fd;



    Player() : master_socket_fd(-1), 
               id(0), 
               players(0), 
               listen_socket_fd(-1), 
               next_fd(-1), 
               prev_fd(-1) {}


    ~Player() {
        close(master_socket_fd);
        close(next_fd);
        close(prev_fd);
        close(listen_socket_fd);
    }

    void initialServerforPlayer(const char* hostname){
        int status;
        struct addrinfo host_info;
        struct addrinfo *host_info_list;

        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family   = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        host_info.ai_flags    = AI_PASSIVE; 
        
        const char* port = NULL;
        // cerr << "error 1" << endl;
        status = getaddrinfo(hostname, port, &host_info, &host_info_list);
        if (status != 0){
            cerr << "Error: cannot get address info for host" << endl;
            exit(EXIT_FAILURE);
        }
        
        struct sockaddr_in *addr = (struct sockaddr_in *)(host_info_list->ai_addr);
        addr->sin_port = 0;
        listen_socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                  host_info_list->ai_protocol);
        if (listen_socket_fd == -1) {
            std::cerr << "Error: cannot create socket" << std::endl;
            exit(EXIT_FAILURE);
        }

        int yes = 1;
        status = setsockopt(listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        status = bind(listen_socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1) {
            std::cerr << "Error: cannot bind socket" << std::endl;
            exit(EXIT_FAILURE);
        }

        status = listen(listen_socket_fd, 100);
        if (status == -1) {
            std::cerr << "Error: cannot listen on socket" << std::endl;
            exit(EXIT_FAILURE);
        }
        freeaddrinfo(host_info_list);
        
    }

    int getport(){
        struct sockaddr_in addr_in;
        socklen_t len = sizeof(addr_in);
        if (getsockname(listen_socket_fd, (struct sockaddr *)&addr_in, &len) == -1) {
            cerr << "Error: cannot get socket Port" << endl;
            exit(EXIT_FAILURE);
        }
        return ntohs(addr_in.sin_port);
    }

    void connectMaster(const char* hostname, const char* port){
        connection(hostname, port, master_socket_fd);
        recv(master_socket_fd, &id, sizeof(id), 0);
        recv(master_socket_fd, &players, sizeof(players), 0);
        // then the player becomes a server
        initialServerforPlayer(hostname);
        int portNum = getport();
        //send the player's listen port to ringmaster
        send(master_socket_fd, &portNum, sizeof(portNum), 0);
        cout << "Connected as player " << id << " out of " << players << " total players"<< endl;
    }

    void connection(const char *hostname, const char *port, int &socket_fd){
        int status;
        struct addrinfo host_info;
        struct addrinfo *host_info_list;

        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        // cerr << "error 2" << endl;
        status = getaddrinfo(hostname, port, &host_info, &host_info_list);
        if (status != 0){
            cerr << "Error: cannot get address info" << endl;
            exit(EXIT_FAILURE);
        }
        socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
        if (socket_fd == -1) {
            cerr << "Error: cannot create socket" << endl;
            exit(EXIT_FAILURE);
        }
        status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1) {
            cerr << "Error: cannot connect to socket" << endl;
            exit(EXIT_FAILURE);
        }
        freeaddrinfo(host_info_list);
    }

    void connectNeighbors(){        
        int length_ip;
        recv(master_socket_fd, &length_ip, sizeof(length_ip), 0);
        char next_ip[length_ip + 1];
        memset(next_ip, 0, sizeof(next_ip));
        recv(master_socket_fd, next_ip, length_ip, 0);
        next_ip[length_ip] = '\0';

        int next_port;
        recv(master_socket_fd, &next_port, sizeof(next_port), 0);
        string port_str = to_string(next_port);
        const char* c_port = port_str.c_str();
        // cerr << "error 3" << endl;
        //std::cout << "next_ip: " << next_ip << std::endl;
        //std::cout << "next_port (int): " << next_port << std::endl;
        //std::cout << "c_port (const char*): " << c_port << std::endl;

        // connect to next
        connection(next_ip, c_port, next_fd);
        
        // accept previous connection
        acceptConnection();

    }

    void acceptConnection() {
        struct sockaddr_storage prev_addr;
        socklen_t prev_addr_len = sizeof(prev_addr);
        prev_fd = accept(listen_socket_fd, (struct sockaddr*)&prev_addr, &prev_addr_len);
        if(prev_fd == -1){
            cerr << "Error: cannot accept connection on socket" << endl;
            exit(EXIT_FAILURE);
        }
    }

    void playHotPotato(){
        srand((unsigned int)time(NULL)+id);
        Potato potato;
        fd_set readfds;
        int fd_arr[3] = {master_socket_fd, prev_fd, next_fd};
        // find the largest
        int max_fd = fd_arr[0];
        for (int i = 1; i < 3; i++) {
            if (fd_arr[i] > max_fd) {
                max_fd = fd_arr[i];
            }
        }
        int n = max_fd + 1;
        bool runGame = true;
        while (runGame){
            FD_ZERO(&readfds);
            for (int i = 0; i < 3; i++) {
                FD_SET(fd_arr[i], &readfds);
            }
            int rv = select(n, &readfds, NULL, NULL, NULL);
            if (rv == -1){
                cerr << "Error in select!" << endl;
                exit(EXIT_FAILURE);
            } 
            int i = 0;
            for (i=0; i<3; i++) {
                if (FD_ISSET(fd_arr[i], &readfds)) {
                    recv(fd_arr[i], &potato, sizeof(potato), 0);
                    //cout << i << fd_arr[i] << endl;
                    break;
                }
            }
            
            if (potato.hops != 0 || i != 0){
                potato.hops = potato.hops - 1;
                potato.size = potato.size + 1;
                potato.trace[potato.size - 1] = id;
                
                if (potato.hops == 0) { // no need to send to next
                    send(master_socket_fd, &potato, sizeof(potato), 0);
                    cout << "I'm it" << endl;
                } else {
                    // send to next
                    
                    int randNum = rand() % 2;
                    int playNum;
                    if (randNum == 0){ // to previous
                        playNum = (id - 1 + players) % players;
                        cout << "Sending potato to " << playNum << endl;
                        send(prev_fd, &potato, sizeof(potato), 0);
                    } else{
                        playNum = (id + 1) % players;
                        cout << "Sending potato to " << playNum << endl;
                        send(next_fd, &potato, sizeof(potato), 0);
                    }
                }
            }else { // end game: hop=0 and from master fd
                runGame = false;
                return;
            }
            
            
        }

    }
};



int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "The input should be: ./player <machine_name> <port_num>" << endl;
    };
    Player *player = new Player();
    player->connectMaster(argv[1],argv[2]);
    player->connectNeighbors();
    player->playHotPotato();
    
    delete player;
    return EXIT_SUCCESS;
}