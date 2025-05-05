

#include "webserv.hpp"


const std::string	*parseRess( std::string filename )
{
	std::string		*content = new std::string;
	std::string		buff;
	char			spaces[7] = {9, 10, 11, 12, 13, 14, 0};
	std::ifstream	ifs(filename);
	unsigned int	k = 0;
	
	if (!ifs.is_open())
	{
		content->assign("");
		return (content);
	}
	while (std::getline(ifs, buff))
		*content += buff;
	while (k < content->length())
	{
		k = content->find_first_of(spaces);
		if (k >= content->length())
			break;
		content->erase(k, 1);
	}
	ifs.close();
	return (content);
}
