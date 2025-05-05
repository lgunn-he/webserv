
#include "webserv.hpp"



ServInstance::ServInstance( void ): hservs(), cli() {

    //this->fds.fd = 0;

    //this->fds.reserve(1);
    //this->fds.resize(1);
    //std::cerr << "ServInstance default construct called!" << std::endl;
	//servaddr.sin_len = 0;
	//servaddr.sin_family = 0;
	//servaddr.sin_port = 0;
	return ;
}

ServInstance::ServInstance( t_server serv ) {

	const int flag = 1;
    struct pollfd tmp;
    struct sockaddr_in  servaddr;

	//this->sockfd.fd = socket(AF_INET, SOCK_STREAM, 0);
	//if (this->sockfd.fd == -1)
		//throw std::runtime_error("Socket creation failed");
    // insert one?
    //std::cerr << "ServInstance construct with serv called!" << std::endl;
    tmp.events = POLLIN;
    tmp.revents = 0;
    tmp.fd = -1;
    //this->fds.reserve(1);
    //this->fds.resize(1);
    this->fds.push_back(tmp);
    this->fds[0].fd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->fds[0].fd == -1)
		throw std::runtime_error("Socket creation failed");
    //this->fds[0].events = POLLIN;
	if (fcntl(this->fds[0].fd, F_SETFL, fcntl(this->fds[0].fd, F_GETFL, 0) | O_NONBLOCK) == -1
			|| setsockopt(this->fds[0].fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)))
	{
		close(this->fds[0].fd);
		throw std::runtime_error("Flag setting failed");
	}

	if (!serv.host.compare("*"))
	{
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else
    {
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = inet_addr(serv.host.c_str());
    } 

	servaddr.sin_port = htons(serv.port);
	if ((bind(this->fds[0].fd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
	{
		std::cerr << std::strerror(errno) << std::endl;
		throw std::runtime_error("Socket binding failed");
	}
    if (listen(this->fds[0].fd, 100) != 0)
        throw std::runtime_error("Could not listen on socket");
    this->prio = false; // faut qu'il soit set à false normalement
	return ;
}

ServInstance::ServInstance( const ServInstance &inst ) {

	*this = inst;
    //std::cerr << "ServInst copy construct called!" << std::endl;
	return ;
}

const ServInstance	&ServInstance::operator=( const ServInstance &inst ) {

	//this->fds = inst.sockfd.fd;	// breeeeh... Faut faire Ca mais actuellement flemme
    this->fds = inst.fds;
	//servaddr = inst.servaddr; //...
	this->hservs = inst.hservs;
	this->cli = inst.cli;
    this->prio = inst.prio;
    //std::cerr << "ServInst assignement operator called!" << std::endl;
	return (inst);
}

ServInstance::~ServInstance( void ) {
  
    //std::cerr << "ServInst destruct called!" << std::endl;
	return ;
}



