#include <cstring>
#include <string>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <exception>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>


void    formatMessage( std::string &message )
{
    int pos = message.find("\n");

    while (pos != std::string::npos)
    {
        message.replace( pos , 1, "\r\n");
        pos = message.find("\n", pos + 2);
    }
    return ;
}

void    setupConnection( int *sock, sockaddr_in *serverAddress, char *arg)
{
    std::string     *tmp;
    const int       flag = 1;
    
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock < 0)
        throw std::exception();

    tmp = new std::string(arg);
    serverAddress->sin_family = AF_INET;
    serverAddress->sin_port = htons(std::stoi(tmp->substr(tmp->rfind(":") + 1)));
    *tmp = tmp->substr(tmp->find("://") + 3);
    *tmp = tmp->substr(0, tmp->find(":"));
    serverAddress->sin_addr.s_addr = inet_addr(tmp->c_str());
    delete tmp;
    if (fcntl(*sock, F_SETFL, fcntl(*sock, F_GETFL, 0) |O_NONBLOCK) == -1
        || setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)))
        throw std::exception();

    while (connect(*sock, (struct sockaddr*)serverAddress, sizeof(*serverAddress)) < 0
            && errno == EINPROGRESS)
    {
        errno = 0;
        usleep(250000);
    }
    if (errno != EISCONN && errno != EINPROGRESS && errno != 0)
        throw std::exception();
    return ;
}

int main(int argc, char **argv)
{
    int             sock;
    struct pollfd   sckt;
    sockaddr_in     serverAddress;
    std::ifstream   ifs;
    char            respbuffer[1025];
    std::string     buff = "";
    std::string     message = "";
    std::string     mescopy = "";



    if (argc != 3)
    {
        std::cerr << "Error: Program takes a url and a request template" << std::endl;
        return (1);
    }
    try {
        setupConnection(&sock, &serverAddress, argv[1]);
    } catch (std::exception &e) {
        std::cerr << "Error: Fatal error when setting up connection" << std::endl;
        return (2);
    }


    ifs.open(argv[2]);
    if (!ifs.is_open())
    {
        std::cerr << "Error: Can't open file" << std::endl;
        close(sock);
        return (3);
    }
    do
    {
        message.append(buff);
        std::getline(ifs, buff);
        buff += '\n';
    } while (ifs.good());
    ifs.close();

    if (message.empty())
    {
        std::cerr << "Error: Empty message" << std::endl;
        close(sock);
        return (4);
    }
    mescopy = message;
    formatMessage(message);

    sckt.fd = sock;
    sckt.events = POLLIN |POLLOUT;
    do
    {
        poll(&sckt, 1, 100);
    } while (!(sckt.revents & POLLOUT));
    if (send(sock, message.c_str(), message.length(), 0) != message.length())
    {
        std::cerr << "Error: Error while sending message" << std::endl;
        close(sock);
        return (5);
    }
    message.clear();
    do
    {
        poll(&sckt, 1, 100);
        if (!(sckt.revents & POLLIN))
            continue ;
        if (recv(sock, respbuffer, 1024, 0) <= 0)
            break ;
        usleep (250000);
        message.append(respbuffer) ;
        bzero(respbuffer, 1024);
        poll(&sckt, 1, 100);
        if (!(sckt.revents & POLLIN))
            break ;
    } while (1);

    std::cout << "===================================="  << "\nRequest to " << argv[1] << std::endl << "====================================" << std::endl;
    if (mescopy.length() > 1000)
    {
        mescopy.substr(0, 994);
        mescopy.append(" [...]");
    }
    std::cout << mescopy << std::endl;
    std::cout << "===================================="  << "\nResponse from " << argv[1] << std::endl << "====================================" << std::endl;
    std::cout << message << std::endl;
    close(sock);
    
    return (0);
}

