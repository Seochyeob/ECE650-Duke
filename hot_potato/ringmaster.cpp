#include <cstdio>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <string>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/times.h>
#include <time.h>


#include "potato.h"

using namespace std;


class Ringmaster{
public:
    int players;
	int hops;
    int socket_fd;
    vector<int> fd_vec;
    vector<string> ip_vec;
    vector<int> port_vec;

    Ringmaster(char** args): players(atoi(args[2])), hops(atoi(args[3])), socket_fd(-1){ }

    ~Ringmaster(){	    
	    for(size_t i = 0; i < fd_vec.size(); i++){
	        close(fd_vec[i]);
	    }
        close(socket_fd);
	}

    void initialServer(const char* port){
        int status;
        struct addrinfo host_info;
        struct addrinfo *host_info_list;
        const char *hostname = NULL;

        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        host_info.ai_flags = AI_PASSIVE;

        status = getaddrinfo(hostname, port, &host_info, &host_info_list);
        if (status != 0){
            cerr << "Error: cannot get address info for host" << endl;
            exit(EXIT_FAILURE);
        }

        socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
        if (socket_fd == -1) {
            cerr << "Error: cannot create socket" << endl;
            exit(EXIT_FAILURE);
        }

        int yes = 1;
        status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1){
            cerr << "Error: cannot bind socket" << endl;
            exit(EXIT_FAILURE);
        }

        status = listen(socket_fd, 100);
        if (status == -1) {
            cerr << "Error: cannot listen on socket" << endl; 
            exit(EXIT_FAILURE);
        }

        freeaddrinfo(host_info_list);
    }

    void connectPlayers(){
        for (int i = 0; i < players; i++) {
            struct sockaddr_storage socket_addr;
            socklen_t socket_addr_len = sizeof(socket_addr);
            int fd_num = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
            if (fd_num == -1) {
                cerr << "Error: cannot accept connection on socket" << endl;
                exit(EXIT_FAILURE);
            }
            fd_vec.push_back(fd_num);
            // get ip addresses
            struct sockaddr_in* tmp_ptr = (struct sockaddr_in*)(&socket_addr);
            string ip_num = inet_ntoa(tmp_ptr->sin_addr);
            ip_vec.push_back(ip_num);
            //send player id and number of player
            send(fd_vec[i], &i, sizeof(i), 0);
            send(fd_vec[i], &players, sizeof(players), 0);
            //receive player's port number
            int port_num;
            recv(fd_vec[i], &port_num, sizeof(port_num), 0);
            port_vec.push_back(port_num);
            cout << "Player " << i << " is ready to play" << endl;
        }
        // send next neighbors' info
        for(int i = 0; i < players; i++){
            int index = (i + 1) % players;
            int length_ip = ip_vec[index].length();
            //send ip length
            send(fd_vec[i], &length_ip, sizeof(length_ip), 0);
            //send ip address
            //std::cout << "next_ip (ringmaster): " << ip_vec[index].c_str() << std::endl;
            send(fd_vec[i], ip_vec[index].c_str(), length_ip, 0);
            //send port num
            //std::cout << "next_port (ringmaster): " << port_vec[index] << std::endl;
            send(fd_vec[i], &port_vec[index], sizeof(port_vec[index]), 0);
        }
    }


    void receivePotato(){
        Potato potato;
        fd_set readfds;
        FD_ZERO(&readfds);
        for(int i = 0; i < players; i++){
            FD_SET(fd_vec[i], &readfds);
        }
        int n = 0;
        if (!fd_vec.empty()) {
            n = fd_vec.back();
        }
        n = n + 1;
        int rv = select(n, &readfds, NULL, NULL, NULL);
        if (rv == -1){
            cerr << "Error in select!" << endl;
            exit(EXIT_FAILURE);
        } else if(rv == 0){
            cerr << "Timeout occurred in select!" << endl;
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < players; i++){
            if (FD_ISSET(fd_vec[i], &readfds)){
                recv(fd_vec[i], &potato, sizeof(potato), NULL);
                // send end signal
                for(int j=0; j<players; j++){
                    send(fd_vec[j], &potato, sizeof(potato), 0);
                }

                break;
            }
        }

        // end of game
        potato.print_traces();
    }

    void playPotato(){
        Potato potato;
        potato.hops = hops;
        // if hops is zero
        if (hops == 0) { // end game
            for (int i = 0; i < players; i++) {
                send(fd_vec[i], &potato, sizeof(potato), 0);
            }
            return;
        }
        // generate random numbers
        srand((unsigned int)time(NULL));
        int randNum = rand() % players;
        cout << "Ready to start the game, sending potato to player " << randNum << endl;
        send(fd_vec[randNum], &potato, sizeof(potato), 0);
        // receive potato
        receivePotato();
    }
};




int main(int argc, char **argv) {
    if (argc != 4) {
        cout << "The input should be: ./ringmaster <port_num> <num_players> <num_hops>" << endl;
    }
    int player = atoi(argv[2]);
    int hop = atoi(argv[3]);
    if (player <= 1 || hop < 0 || hop > 512){
        cout << "Wrong input number\n" << endl;
        exit(EXIT_FAILURE);
    }
    Ringmaster *ringmaster = new Ringmaster(argv); // new Ringmaster, check num_player larger than 1, 0 <= num_hops <= 512
    // print initial
    cout << "Potato Ringmaster" << endl;
    cout << "Players = " << player << endl;
    cout << "Hops = " << hop << endl;
    const char *port = argv[1];
    ringmaster->initialServer(port);
    ringmaster->connectPlayers();
    ringmaster->playPotato();
    delete ringmaster;
    return EXIT_SUCCESS;
}