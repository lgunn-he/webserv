
#include "webserv.hpp"

int SIG_HANDLER_GLOBAL = 0;

int fd_is_valid(int fd)
{
    errno = 0;
    return (fcntl(fd, F_GETFD) != -1 || errno != EBADF);
}

bool    isSocket (int fd)
{
    struct stat statbuf;

    fstat(fd, &statbuf);
    return (S_ISSOCK(statbuf.st_mode));
}

void    sig_handle( int sig )
{
    SIG_HANDLER_GLOBAL = sig;
    std::cerr << sig << std::endl;
    return ;
}

void    handle_args( int argc, char **argv, std::string &path)
{
    if (argc == 1)
        path = DEF_CONF_PATH;
    else if (argc == 2)
        path = std::string(argv[1]);
    else
        throw std::runtime_error("Program takes at most one argument");
    return ;
}

int launch_configparser( std::string tmp, std::vector<t_server> &servs)
{
    
    try {parseConfigFile(tmp, servs);}
    catch(unset_exception &e) {

        std::cerr << "\33[35;1mWarning:\33[0m some values have not been set correctly in the config file." << std::endl;
        do
        {
            std::cerr << "These values will be set to default. Do you wish to continue? (y/n): " ;
            tmp = "";
            std::cin >> tmp;
        } while (tmp.compare("y") && tmp.compare("n"));
        if (!tmp.compare("n"))
            return (3);
        std::cout << std::endl;
    } catch (std::exception &e) {
        
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "***" << std::endl;
        return (2);
    }
    return (0);
}

int warn_ip_loop(  std::string tmp, std::vector<t_server> &servs )
{

    for (std::vector<t_server>::const_iterator it = servs.begin(); it != servs.end(); it ++)
    {
        if (it->host.compare("127.0.0.1") && it->host.compare("*"))
        {
            std::cerr << "\33[35;1mWarning:\33[0m The host ip is set to a value different from the loopback address." << std::endl;
            do
            {
                std::cerr << "Please ensure this ip is a valid interface on this machine. (y/n): " ;
                tmp = "";
                std::cin >> tmp;
            } while (tmp.compare("y") && tmp.compare("n"));
            if (!tmp.compare("n"))
                return (4);
            std::cout << std::endl;
        }
    }
    return (0);
}

void    check_pythonpath( void )
{
    std::string tmp;

    if (access(PYTHON_PATH, X_OK))
        throw std::runtime_error("Invalid python3 path");
    tmp = PYTHON_PATH;
    if (tmp.find("python3") != tmp.length() - 7)
        throw std::runtime_error("Invalid python3 path");
    return ;
}

void    check_confexists( std::string tmp )
{
    std::string confp = DEF_CONF_PATH;

    if (access(DEF_CONF_PATH, F_OK | R_OK))
        throw std::runtime_error("You have invalidated the default config file. Please provide a valid config file named " + confp + " to use this program");
    if (access(tmp.c_str(), F_OK | R_OK))
        throw std::runtime_error("Config file either does not exist or cannot be read");
    return ;
}

bool    perform_checks( int argc, char **argv, std::string &tmp, std::vector<t_server> &servs)
{
    try
    {
        handle_args(argc, argv, tmp);
        check_pythonpath();
        if (launch_configparser(tmp, servs) || warn_ip_loop(tmp, servs))
            return (true);
    } catch (std::exception &e) {
    
        std::cerr << "Error: " << e.what() << std::endl;
        return (true);
    }
    return (false);
}

int main(int argc, char **argv)
{

    std::vector<t_server>   servs;
    std::string             tmp;

    if (perform_checks(argc, argv, tmp, servs))
        return (1);
    std::cout << "Configuration file OK!" << std::endl;
    std::cout << "Starting webserver..." << std::endl;
    signal(SIGINT, sig_handle);
    try {startserver(servs);} 
    catch (std::exception &e) {std::cerr << "Error: Fatal: " << e.what() << std::endl;}
    std::cout << std::endl << "Stopping webserver..." << std::endl;
    return (0);
}

