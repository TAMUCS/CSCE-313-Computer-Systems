#include "TCPRequestChannel.h"

using namespace std;

#define StoUS(x)  static_cast<unsigned short>(std::strtoul(x.c_str(), NULL, 0))

TCPRequestChannel::TCPRequestChannel(const std::string _ip_address, const std::string _port_no) {
    struct sockaddr_in addr; 
    std::memset(&addr, 0, sizeof(addr)); // LEC24/25PP
    addr.sin_family = AF_INET;
    addr.sin_port = htons(StoUS(_port_no)); // >:(

    // create socket on specified: domain, type, protocol 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // LEC24/25PP

    // use sockaddr_in struct in call to bind(); set port & addr in struct
        // BYTE HANDLING: int inet_pton()
    // On CLIENT side, convert IP addr to the network addr that can be used 
    //                                  in sockaddr_in struct to connect to server
    if (_ip_address.size() == 0) { // SERVER
            addr.sin_addr.s_addr = INADDR_ANY; //LAB11PP
            //bind() socket to addr
            bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)); // BEEJ 
            //mark socket as listening (listen())
            listen(sockfd, 110); // w_max connections + 10 to be safe
        
    } else { // CLIENT
            // connect to socket to the IP address of the server
            inet_pton(AF_INET, _ip_address.c_str(), &addr.sin_addr); // LAB11PP
            connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)); // BEEJ
    }
}

TCPRequestChannel::TCPRequestChannel(int _sockfd) : sockfd(_sockfd) {} // JEJ

TCPRequestChannel::~TCPRequestChannel() { close(sockfd); } // KEK

int TCPRequestChannel::accept_conn() { // BEEJ
    struct sockaddr_in acc;
    socklen_t len(sizeof(acc));
    return accept(sockfd, (struct sockaddr*)&acc, &len);
}

int TCPRequestChannel::cread(void* msgbuf, int msgsize) { return read(sockfd, msgbuf, msgsize); } // KOL

int TCPRequestChannel::cwrite(void* msgbuf, int msgsize) { return write(sockfd, msgbuf, msgsize); } // KEJ
