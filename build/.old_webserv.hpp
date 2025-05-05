
#ifndef	__WEBSERV_HPP__
# define	__WEBSERV_HPP__

# include <iostream>
# include <sys/time.h>
# include <netdb.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <unistd.h>
# include <stdlib.h>
# include <cstring>
# include <string>
# include <fstream>
# include <map>
# include <vector>
# include <exception>
# include <sys/poll.h>
# include <sys/stat.h>
# include <dirent.h>
# include <errno.h>
# include <time.h>
# include <signal.h>
# include <fcntl.h>



#define	MAX_PORT_VALUE	49151
#define	MIN_PORT_VALUE	1024
#define	DEF_MAX_BODY	500000000 //4096
#define	MAX_ERR_PGS		5
#define	MAX_HEAD_SIZE	900
#define CHUNK_SIZE      2000
#define	DEF_CONF_PATH	"../configs/spare_conf"
#define	PYTHON_PATH		"/opt/homebrew/bin/python3"
#define CREATED_FILE_P	S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define	NGINX_LOC_ROOT	false
#define RUN_AS_VERBOSE	false
#define	HTML_ONLY_CGI	false
#define OVERWRITE_POST	false
#define GET_AS_DL       true


extern int	SIG_HANDLER_GLOBAL;


typedef struct	s_location {

	std::string					path;
	std::string					root;
	std::string 				upload_dir;
	std::string					redirect;
	std::vector<std::string>	index;
	std::vector<std::string>	cgi;
	int							methods; // 1 = GET, 2 = POST, 4 =	DELETE
	int							autoindex;
	bool						set_values_flag[8];

								s_location( void );
}								t_location;

typedef struct	s_server { 

	std::string						host;
	std::string						root;
	int								port;
	unsigned long					cli_body_len;
	std::map<int, std::string>		error_pages;
	std::vector<std::string>		server_names;
	std::vector<std::string>		index;
	bool							autoindex;
	std::vector<struct s_location>	loc;
	bool							set_values_flag[8];

	bool							setErrPgs( std::string line);
	bool							setVec( std::string line, std::vector<std::string> &val );
	bool							setMethods( std::string );
	bool							setAutoindex( std::string line, bool );
	bool							setInteger( std::string , int * );
	bool							setInteger( std::string line, unsigned long *val );
	bool							setString( std::string , std::string * );
	bool							setValue( std::string, int, bool * );
	bool							addValue( std::string, bool *, bool * );
									s_server( void );
}									t_server;

typedef struct	s_client {

	std::vector<struct pollfd>	fds;
    std::vector<std::vector<char> >		bin_buffers;
	std::vector<struct timeval>	last_mod;
	std::vector<long>			CRLFpos;
    std::vector<bool>           readReady;
    unsigned long               perce;
}								t_client;

struct	ServInstance {

		int	sockfd;
		struct sockaddr_in servaddr;
		std::vector<t_server>	hservs;
		t_client				cli;
        bool                    prio;

		ServInstance( void );
		ServInstance( t_server );
		ServInstance( const ServInstance & );
		const ServInstance	&operator=( const ServInstance & );
		~ServInstance( void );

};

class	Req {

	private:
		std::string			_method;
		std::string			_ressource;
		std::string			_requested_ressource;
		std::vector<char>	_body;
		std::string			_query;
		std::string			_contentype;
		t_location			chosen_loc;
		bool				keepalive;

	public:
		const std::string		getMethod( void ) const;
		const std::vector<char>	getBody( void ) const;
		const std::string		getRessource( void ) const;
		const std::string		getRequestedRessource( void ) const;
		void					setRessource( std::string ) ;
		const std::string		getQuery( void ) const;
		const std::string		getContentType( void ) const;
		const t_location		&getChosenLoc( void ) const;
		bool					isChosen( void ) const;
		bool					getConnection( void ) const;
		bool					handleLocation( t_server serv );
		Req( void );
		Req( const Req & );
		Req( std::vector<char>, t_server );
		~Req( void );
		const Req &operator=( const Req & );
};

class	Resp {

	private :
		std::string		*_header;
		std::string		*_body;
		char			*_bin_data;
		unsigned int	_binsize;
		unsigned int	_status;
		void		methodHandler( const Req &, const t_server, std::string & );
		int			Get( const Req &, const t_server );
		int			binary_Get( const Req &, const t_server );
		int			Post( const Req &, const t_server &, std::string & );
		int			binary_Post( const Req &, const t_server &, const std::string );
		int			Delete( const Req &, const t_server & );


	public:
		int					launch_cgiscript( const Req	&, t_server );
		int					handle_reqlocproperties( const Req &, const t_server );
		std::string			getContentType( const Req & );
		std::string			formatDateChars( int d, unsigned int zer);
		std::string			getDateString( void );
		std::string			add_headers( const Req &, bool *, std::string & );
		bool				transmit( int, const Req &, std::string & );
		Resp( void );
		Resp( const Resp & );
		Resp( const Req &, const t_server, std::string & );
		~Resp( void );
		const Resp &operator=( const Resp & );

};

class	unset_exception: public std::exception {
	
	private:

	public:
};

class	request_exception: public std::exception {
	
	private:
		int	err;

	public:
		int		getErr( void );
		void	setErr( int );
				request_exception( int );
};


const		std::string	*parseRess( std::string filename );
int		ft_isspace(char c);
void	parseConfigFile( std::string, std::vector<t_server> &);
void	readConfigFile( std::string , std::vector<t_server> &, bool *, int *);
bool	is_closing( std::string );
bool	is_server( std::string );
int		is_servvalue( std::string );
int		is_locvalue( std::string );
void	printConfigError( std::string , int );
void	startserver( std::vector<t_server> );
void	server_loop( std::vector<ServInstance> &, std::vector<t_server> );
bool	isdir( std::string );
void	selectServerInst( std::string , const ServInstance &,  t_server * );
bool	checkHostIP( std::string );
int		exec_cgiscript( std::string, int *, int *, int *, const Req & );
void	set_CGIasbody( std::string **, int * );
void	flush_stdout( void );
void	formatSlash( std::string *, std::string );
std::string	leanLink( std::string link );
void	checkErrVal( int *err );
bool	check_cgibody( std::string *body );
const std::string	errorHead( int );
const std::string	genErrorBody( int err );
void	check_IncomingConnections( int sockfd, std::vector<struct pollfd> &clifds );
long	get_ContentLength( std::string text );
bool	isFullRequest( std::vector<char> & );
void	cycle_readFds( t_client &cli );
unsigned int	get_timediff( struct timeval last );
void	check_timeout( t_client &cli );
const std::string	get_methodfrombuff( std::string );
const std::string	get_bodyfrombuff( std::string );
const std::string	get_httpfrombuff( std::string );
void	check_request_structure( std::string , t_server );
const std::string	parse_query( std::string , int , unsigned long * );
void	close_pipes( int *, int *, int  );
int	open_pipes( int *, int * );
int	parent_proc( int , int *, int *, int *, const Req & );
void	write_reqtocgi( int *, const Req & );
const std::string	parse_contentype( std::string );
const std::string	getErrorBody( const t_server serv, int err );
void	check_pythonpath( void );
bool	perform_checks( int , char **, std::string &, std::vector<t_server> & );
void	check_confexists( std::string );
int	warn_ip_loop( std::string , std::vector<t_server> & );
int	launch_configparser( std::string , std::vector<t_server> & );
void	check_methodstructure( std::string buff );
void	handle_connectionerror( int , t_client &, ServInstance &, int );
void	handle_Request( t_client &cli, ServInstance &inst, int i );
bool	complete_req( t_client &, ServInstance &, int );
std::string	fitLink( std::string link );
std::string	get_strfrombuff( const std::vector<char> & );
void	add_bufftoback( std::vector<char> &, char *, int );
void	sig_handle( int );
std::string	remove_excesSlash( std::string );

template <typename T>
std::string	*handleIndex( const Req &, T, bool, int * );



#endif

