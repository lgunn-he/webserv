

#include "webserv.hpp"

int	request_exception::getErr( void ) {

	return (this->err);
}

void	request_exception::setErr( int err ) {

	this->err = err;
	return ;
}

request_exception::request_exception( int err ) {

	this->err = err;
	return ;
}

void	formatSlash( std::string *dest, std::string ress ) {

	unsigned int i;

	if (!ress.empty())
		dest->insert(0, ress);
	for (i = 0; i < dest->length() && (*dest)[i] == '/'; i++);
	dest->erase(0, i);
	return ;
}

std::string	leanLink( std::string link ) {

	std::string		ret = link;
	unsigned int	i = 0;

	if (link.empty())
		return ("");
	else if (link.find_first_not_of("/") == std::string::npos)
		return ("/");
	while (ret[i] && ret[i ++] == '/');
	i --;
	if (i == ret.length())
		return ("");
	ret.erase(0, i);
	i = ret.length() - 1;
	while (ret[i] && ret[i --] == '/');
	i += 2;
	ret.erase(i, ret.length() - i);
	return (ret);
}

std::string	fitLink( std::string link ) {

	unsigned long	pos = 0;
	unsigned long	n;

	while (1)
	{
		n = 0;
		pos = link.find("//");
	 	if (pos == std::string::npos)
			break;
		n = link.find_first_not_of('/', pos);
		if (n == std::string::npos)
			link.erase(pos + 1);
		else
			link.erase(pos, n - pos - 1);
	}
	return (link);
}

bool	Req::handleLocation( t_server serv ) {

	unsigned long	len = 0;
	t_location	tmp;
	std::string smp;

	for (std::vector<t_location>::const_iterator it = serv.loc.begin(); it != serv.loc.end(); it ++)
	{
		smp = fitLink(it->path);
		if (len < smp.length() && !smp.compare(0, smp.length(), this->_ressource, 0, smp.length()))
		{
			len = smp.length();
			tmp = *it;
		}
	}
	if (len != 0)
	{
		if (!tmp.root.empty())
		{
			if (NGINX_LOC_ROOT == false)
				this->_ressource.erase(0, tmp.path.length());
			this->_ressource.insert(0, tmp.root);
		}
		else
			this->_ressource.insert(0, serv.root);
		this->_ressource = fitLink(this->_ressource);
		this->chosen_loc = tmp;
		return (true);
	}
	return (false);
}

const std::string	Req::getMethod( void ) const {

	return (this->_method);
}

const std::vector<char>	Req::getBody( void ) const {

	return (this->_body);
}

const std::string	Req::getQuery( void ) const {

	return (this->_query);
}

const std::string	Req::getRessource( void ) const {

	return (this->_ressource);
}

const std::string	Req::getRequestedRessource( void ) const {

	return (this->_requested_ressource);
}


void	Req::setRessource( std::string ress ) {

	this->_ressource = ress;
	return ;
}

const std::string	Req::getContentType( void ) const {

	return (this->_contentype);
}

const t_location	&Req::getChosenLoc( void ) const {

	return (this->chosen_loc);
}

bool				Req::isChosen( void ) const {

	if (this->chosen_loc.path.empty())
		return (false);
	return (true);
}

bool	Req::getConnection( void ) const {

	return (this->keepalive);
}

Req::Req( void ) {

	return ;
}

Req::~Req( void ) {

	return ;
}


Req::Req( const Req &inst ) {

	this->_method = inst.getMethod();
	this->_ressource= inst.getRessource();
	return ;
}

const std::string	get_httpfrombuff( std::string buff ) {

	std::string	line = buff.substr(0, buff.find("\r\n"));
	unsigned long	pos = line.find("HTTP/1.1");

	if (pos == std::string::npos ||pos != line.length() - 8)
		return ("");
	return ("yea all good mate");
}

const std::string	get_bodyfrombuff( std::string buff ) {

	std::string		body;
	unsigned long	pos;

	pos = buff.find("\r\n\r\n");
	if (pos == std::string::npos)
		return ("");
	return (buff.substr(pos + 4, buff.length() - (pos + 4)));
}

const std::string	get_methodfrombuff( std::string buff ) {

	std::string method;
	
	if (buff.find(" ") == std::string::npos)
		return ("");
	method = buff.substr(0, buff.find(" "));
	if (method.compare("GET") && method.compare("POST") && method.compare("DELETE"))
		return ("");
	return (method);
}

void	check_request_structure( std::string buff ) {

	unsigned long	pos = 0;
	std::string	line;

	pos = buff.find("\r\n");
	if (pos > 250)
		throw request_exception(400);
	line = buff.substr(0, pos);
	if (line.find("HTTP/") == std::string::npos || line.find("HTTP/") < 6)
		throw request_exception(400);
	if (get_httpfrombuff(buff).empty())
		throw request_exception(505);
	if (get_methodfrombuff(buff).empty())
		throw request_exception(405);
	return ;
}

void	check_methodstructure( std::string buff ) {

	std::string method = get_methodfrombuff(buff);

	if (!method.compare("POST"))	
	{
		if (get_ContentLength(buff) < 0 || buff.length() - buff.find("\r\n\r\n") < 5)
			throw request_exception(400);
	}
	else
	{
		if (get_ContentLength(buff) > 0 || buff.length() - buff.find("\r\n\r\n") > 4)
			throw request_exception(400);
	}
	return ;
}

void	check_allowedmethods( std::string method, int allowed ) {


	if (!method.compare("GET") && (allowed & 1))
		return ;
	if (!method.compare("POST") && (allowed & 2))
		return ;
	if (!method.compare("DELETE") && (allowed & 4))
		return ;
	throw request_exception(405);
	return ;
}

const std::string	parse_query( std::string tmp, int j, unsigned long *l ) {

	if (tmp.find(' ', j) < tmp.find('?', j))
	{
		*l = tmp.find(' ', j);
		return ("");
	}
	*l = tmp.find('?', j);
	return (tmp.substr(*l, tmp.find(' ', *l) - *l));
}

const std::string	parse_contentype( std::string buff ) {

	unsigned long	pos = buff.find("Content-Type: ");
	std::string	line = "";

	if (pos != std::string::npos)
		line = buff.substr(pos, buff.find_first_of(";\r", pos) - pos);
	return (line);
}

Req::Req( const std::vector<char> &buff, t_server &serv ) {

	std::string			tmp = get_strfrombuff(buff);
	unsigned int		j = tmp.find('/');
	unsigned long		l;

	if (RUN_AS_VERBOSE == true)
		std::cout << std::endl << tmp << std::endl;
	check_request_structure(tmp);
	this->_query = parse_query(tmp, j, &l);
	this->chosen_loc.path = "";	
	this->_method.assign(get_methodfrombuff(tmp));
	this->_contentype = parse_contentype(tmp);
	this->_ressource.assign(&(tmp[j]), l - j);
	this->_requested_ressource.assign(&(tmp[j]), l - j);
	this->_ressource = fitLink(this->_ressource);
	this->_requested_ressource = fitLink(this->_requested_ressource);
	if (!handleLocation(serv))
	{
		check_allowedmethods(this->_method, 1);
		formatSlash(&this->_ressource, serv.root);
	}
	else
	{
		check_allowedmethods(this->_method, this->chosen_loc.methods);
		check_methodstructure(tmp);
	}
	if (tmp.find("Connection: ") != std::string::npos 
		&& !tmp.substr(tmp.find("Connection: ") + 12, 12).compare("close\r\n"))
		this->keepalive = false;
	else
		this->keepalive = true; // c'étai set à false ... clairement c'est faux non?
	l = tmp.find("\r\n\r\n") + 4;
	if (l != std::string::npos)
		add_bufftoback(this->_body, &(const_cast<char *>(buff.data())[l]), buff.size() - l);
	return ;
}

const Req	&Req::operator=( const Req &inst ) {

	this->_method = inst.getMethod();
	this->_ressource = inst.getRessource();
	return (inst);
}
