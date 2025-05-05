#include "webserv.hpp"


int	ft_isspace(char c)
{
	if ((c >= 9 && c <= 13) || c == ' ')
		return (1);
	return (0);
}

s_server::s_server( void ) {

	this->host = "127.0.0.1";
	this->port = 80;
	this->root = "/";
	this->cli_body_len = DEF_MAX_BODY;
	this->autoindex = false;
	for (int i = 0; i < 8; i ++)
		this->set_values_flag[i] = false;
}

s_location::s_location( void ) {

	this->path = "!UNSET_!VALUE***";
	this->autoindex = -1;
	this->methods = 1;
	for (int i = 0; i < 8; i ++)
		this->set_values_flag[i] = false;
    this->index.shrink_to_fit();
    this->cgi.shrink_to_fit();
}

bool	checkHostIP( std::string hostip ) {

	unsigned int	points = 0;
	float			initpos = 0;
	float			nextpos;
	unsigned int	digit;


	if (!hostip.compare("*"))
		return (true);
	if (hostip.find_first_not_of("0123456789.") != std::string::npos)
		return (false);
	while (initpos != std::string::npos)
	{
		nextpos = hostip.find('.', initpos);
		points ++;
		if (nextpos == std::string::npos && points != 4)
			return (false);
		try
		{
			if (nextpos == std::string::npos)
				digit = stoi(hostip.substr(initpos, (hostip.length() - initpos)));
			else
				digit = stoi(hostip.substr(initpos, (nextpos - initpos)));
		} catch (std::exception &e) {

			return (false);
		}
		if ((points == 1 && digit > 255)
			|| (points == 2 && digit > 255)
			|| (points == 3 && digit > 255)
			|| (points == 4 && digit > 255))
			return (false);
		initpos = nextpos + 1;
	}
	return (true);
}

bool	s_server::setString( std::string line, std::string *val ) { 

	std::size_t	pos = line.find(':') + 2;

	if ((line.find(' ', pos) != std::string::npos)
			|| !line.substr(pos).compare("!UNSET_!VALUE***"))
		return (false);
	*val = line.substr(pos);
	return (true);
}

bool	s_server::setInteger( std::string line, int *val ) { 

	std::size_t	pos = line.find(':') + 2;

	if (line.find_first_not_of("0123456789", pos) != std::string::npos)
		return (false);
	try
	{
		*val = stoi(line.substr(pos));
	} catch (std::exception &e) {

		return (false);
	}
	return (true);
}

bool	s_server::setInteger( std::string line, unsigned long *val ) { 

	std::size_t	pos = line.find(':') + 2;

	if (line.find_first_not_of("0123456789", pos) != std::string::npos)
		return (false);
	try
	{
		*val = stol(line.substr(pos));
	} catch (std::exception &e) {

		return (false);
	}
	return (true);
}

bool	s_server::setAutoindex( std::string line, bool mode ) {

	std::size_t	pos = line.find(':') + 2;

	if (line.find(' ', pos) != std::string::npos)
		return (false);
	if (mode == true)
	{
		if (!line.substr(pos).compare("on"))
			this->autoindex = true;
		else if (!line.substr(pos).compare("off"))
			this->autoindex = false;
		else
			return (false);
	}
	else
	{
		if (!line.substr(pos).compare("on"))
			this->loc.back().autoindex = 1;
		else if (!line.substr(pos).compare("off"))
			this->loc.back().autoindex = 0;
		else
			return (false);
	}
	return (true);
}

bool	s_server::setMethods( std::string line ) { 

	std::size_t	pos = line.find(':') + 2;
	std::string buff;
	int		mthds[3] = {0};

	while (pos != std::string::npos)
	{
		buff = line.substr(pos, line.find(' ', pos) - pos);
		if (buff.empty())
			break;
		if (!buff.compare("GET") && mthds[0] == 0)
			mthds[0] ++;
		else if (!buff.compare("POST") && mthds[1] == 0)
			mthds[1] ++;
		else if (!buff.compare("DELETE") && mthds[2] == 0)
			mthds[2] ++;
		else
			return (false);
		pos = line.find(' ', pos);
		if (pos == std::string::npos)
			break ;
		pos += 1;
	}

	this->loc.back().methods = 0;
	this->loc.back().methods += mthds[0];
	this->loc.back().methods += 2 * mthds[1];
	this->loc.back().methods += 4 * mthds[2];
	return (true);
}

bool	s_server::setVec( std::string line, std::vector<std::string> &val ) {

	std::size_t		pos = line.find(':') + 1;
	std::string 	buff;
	unsigned int	count = 0;

	do	
	{
		if (count >= 5)
			return (false);
		pos ++;
		buff = line.substr(pos, line.find(' ', pos) - pos);
        if (buff.empty())
            break ;
		if (buff.front() == '/' ||buff.back() == '/')
			return (false);
		val.push_back(buff);
		pos = line.find_first_of(" ", pos);
		count ++;
	} while (pos != std::string::npos);
	return (true);
}

bool	s_server::setErrPgs( std::string line ) { 

	std::size_t	pos = line.find(':') + 2;
	std::string	buff;
	std::pair<int, std::string> tmp;
	int	max_n = 0;

	while (1)
	{
		if (line.find(' ', pos) != std::string::npos)
			buff = line.substr(pos, line.find(' ', pos) - pos);
		else
			buff = line.substr(pos);
		if (buff.empty())
			break;
		if (max_n >= MAX_ERR_PGS)
			return (false);
		tmp.first = stoi(buff.substr(0, buff.find(':')));
		tmp.second = buff.substr(buff.find(':') + 1);
		if (this->error_pages.find(tmp.first) != this->error_pages.end())
			return (false);
		this->error_pages.insert(tmp);
		pos = line.find(' ', pos);
		if (pos == std::string::npos)
			break;
		pos += 1;
		max_n ++;
	}
	return (true);
}

bool	s_server::setValue( std::string line, int structcode, bool *defaults ) {

	std::size_t	pos = 0;
	std::string	buff;
	bool		err;

	pos = line.find(':');
	if (pos == std::string::npos)
		return (false);
	buff.assign(line, pos, std::string::npos);
	if (structcode == 1) 
	{
		err = setString(line, &(this->host));
		defaults[0] = true;
		if (checkHostIP(this->host) != 1)
			return (false);
	}
	else if (structcode == 2) 
	{
		err = setInteger(line, &(this->port));
		defaults[1] = true;
		if (this->port > MAX_PORT_VALUE)
			return (false);
	}
	else if (structcode == 3)
		err = setVec(line, this->server_names);
	else if (structcode == 4) 
	{
		err = setString(line, &(this->root));
		if (!this->root.compare("/"))
			this->root = ".";
		else if (this->root.find(".") != std::string::npos)
			return (false);
		if (this->root[0] != '/')
			this->root.insert(0, "/");
		if (this->root.back() != '/')
			this->root.insert(this->root.length(), "/");
		defaults[2] = true;
	}
	else if (structcode == 5)
	{
		err = setInteger(line, &(this->cli_body_len));
		defaults[3] = true;
	}
	else if (structcode == 6)
		err = setErrPgs(line);
	else if (structcode == 7)
		err = setAutoindex(line, true);
	else if (structcode == 8) 
		err = setVec(line, this->index); 
	else if (structcode == 10) 
	{
		err = setString(line, &(this->loc.back().path));
		if (this->loc.back().path[0] != '/')
			err = false;
	}
	else if (structcode == 11) 
	{
		err = setString(line, &(this->loc.back().root));
		if (this->root[0] != '/')
			this->root.insert(0, "/");
		if (this->root.back() != '/')
			this->root.insert(this->root.length(), "/");
	}
	else if (structcode == 12) 
		err = setMethods(line);
	else if (structcode == 13) 
		err = setVec(line, this->loc.back().cgi);
	else if (structcode == 14) 
		err = setVec(line, this->loc.back().index);
	else if (structcode == 15) 
	{
		err = setString(line, &(this->loc.back().upload_dir));
	}
	else if (structcode == 16) 
		err = setAutoindex(line, false);
	else if (structcode == 17) 
		err = setString(line, &(this->loc.back().redirect));
	else
		return (false);
	return (err);
}

bool	s_server::addValue( std::string line, bool *locval, bool *defaults ) {

	int	structcode = 0;

	if (*locval == true)
	{
		structcode = is_locvalue(line);
		if (structcode != 0 && this->loc.back().set_values_flag[structcode - 10] == false && setValue(line, structcode, defaults));
		else if (is_closing(line))
		{
			if (!this->loc.back().path.compare("!UNSET_!VALUE***"))
				return (false);
			*locval = false;
		}
		else
			return (false);
        if (structcode != 0)                // WARNING, this should be fixed...
		    this->loc.back().set_values_flag[structcode - 10] = true;
	}
	else
	{
		structcode = is_servvalue(line); 
		if (structcode == 9)
		{
			*locval = true;
			this->loc.resize(this->loc.size() + 1);
		}
		else if (structcode != 0 && this->set_values_flag[structcode - 1] == false && setValue(line, structcode, defaults));
		else
			return (false);
		if (structcode != 9)
			this->set_values_flag[structcode - 1] = true;
	}
	return (true);
}

void	printConfigError( std::string line, int line_number ) {

	std::cerr << "\33[31;1mError:\33[0m on line: " << line_number << std::endl;
	std::cerr << '\t' <<  line.substr(line.find_first_not_of('\t')) << std::endl;
	return ;
}

int	is_locvalue( std::string line ) {

	if (line[0] != '\t' || line[1] != '\t')
		return (0);
	if (!line.compare(2, 6, "path: "))
		return (10);
	else if (!line.compare(2, 6, "root: "))
		return (11);
	else if (!line.compare(2, 9, "methods: "))
		return (12);
	else if (!line.compare(2, 5, "cgi: "))
		return (13);
	else if (!line.compare(2, 7, "index: "))
		return (14);
	else if (!line.compare(2, 8, "upload: "))
		return (15);
	else if (!line.compare(2, 11, "autoindex: "))
		return (16);
	else if (!line.compare(2, 10, "redirect: "))
		return (17);
	else
		return (0);
}

int	is_servvalue( std::string line ) {

	if (line[0] != '\t')
		return (0);
	else if (!line.compare(1, 6, "host: "))
		return (1);
	else if (!line.compare(1, 8, "listen: "))
		return (2);
	else if (!line.compare(1, 7, "names: "))
		return (3);
	else if (!line.compare(1, 6, "root: "))
		return (4);
	else if (!line.compare(1, 8, "length: "))
		return (5);
	else if (!line.compare(1, 7, "error: "))
		return (6);
	else if (!line.compare(1, 11, "autoindex: "))
		return (7);
	else if (!line.compare(1, 7, "index: "))
		return (8);
	else if (!line.compare(1, std::string::npos, "location {"))
		return (9);
	else
		return (0);
}

bool	is_server( std::string line ) {

	for (std::size_t i = 8; i < line.length(); i ++)
	{
		if (!ft_isspace(line[i]))
			return (false);
	}
	if (!line.compare(0, 8, "server {"))
		return (true);
	return (false);
}

bool	is_closing( std::string line ) {

	bool bracket_flag = false;

	for (std::size_t i = 0; i < line.length(); i ++)
	{
		if (bracket_flag == false && line[i] == '}')
			bracket_flag = true;
		else if ((bracket_flag = true && line[i] == '}') || (!ft_isspace(line[i]) && line[i] != '}'))
			return (false);
	}
	if (bracket_flag == true)
		return (true);
	return (false);
}

bool    checkPortNoRep( std::vector<t_server> &li ) {

    int selport = 0;
    std::string hst = "";

    if (li.size() < 2)
        return (false);
    selport = li.back().port;
    hst = li.back().host;
    for (int i = 0; i < static_cast<int>(li.size()) - 1; i ++)
    {
        if (li[i].port == selport)
        {
            if (!STRICT_PORTS && !li[i].host.compare(hst))
                continue ;
            return (true);
        }
    }
    return (false);
}

void	readConfigFile( std::string filepath, std::vector<t_server> &server_list, bool *defaults, int *error_flag ) {

	std::ifstream	ifs;
	std::string		buff = "";
	bool			server_flag = false;
	bool			locval = false;
	int				line = 0;

	ifs.open(filepath);
	if (!ifs.is_open())
		throw std::runtime_error("Invalid file name");
	while (!ifs.eof())
	{
		std::getline(ifs, buff);
		line ++;
		if (buff.empty())
			continue ;	
		if ((server_flag == true && (locval == true || !is_closing(buff)))
			&& server_list.back().addValue(buff, &locval, defaults));
		else if (is_server(buff) && server_flag == false)
		{
			for (int i = 0; server_list.size() > 0 && i < 4; i ++)
			{
				if (defaults[i] == false)
					defaults[4] = true;
				else
					defaults[i] = false;
			}
			server_list.resize(server_list.size() + 1);
			server_flag = true;
		}
		else if (is_closing(buff) && locval == false)
		{
			server_flag = false;
			if (server_list.back().port < MIN_PORT_VALUE && server_list.back().host.compare("*"))
			{
				std::cerr << "\33[31;1mError:\33[0m port is in reserved range for host ip" << std::endl;
				*error_flag += 1;
			}
            else if (checkPortNoRep(server_list))
            {
				std::cerr << "\33[31;1mError:\33[0m port is already in use" << std::endl;
				*error_flag += 1;
            }
		}
		else
		{
			if (is_closing(buff) && locval == true)
				locval = false;
			printConfigError(buff, line);
			*error_flag += 1;
		}
	}
	if (server_flag == true)
	{
		std::cerr << "\33[31;1mError:\33[0m on line: " << line << std::endl;
		std::cerr << '\t' << "server tag has not been closed" << std::endl;
		*error_flag += 1;
	}
	if (buff.empty() && line == 1)
		throw std::runtime_error("Empty file");	
	return ;
}

void	parseConfigFile( std::string filepath, std::vector<t_server> &server_list) {

	std::string		buff;
	bool			defaults[5] = {false};
	int				error_flag = 0;

	readConfigFile(filepath, server_list, defaults, &error_flag);
	if (error_flag)
	{
		std::cerr << std::endl;
		buff.append(std::to_string(error_flag));
		if (error_flag == 1)
			buff.append(" error");
		else
			buff.append(" errors");
		buff.append(" generated in file ");
		buff.append(filepath);
		throw std::runtime_error(buff);
	}
	else
		for (int i = 0; i < 4; i ++)
			if (defaults[i] == false)
				defaults[4] = true;
	if (defaults[4])
		throw unset_exception();
	return ;
}
