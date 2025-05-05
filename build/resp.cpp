#include "webserv.hpp"


std::string remove_excesSlash( std::string str )
{
    size_t  i = 0;

    if (str.empty())
        return ("");
    while (i != std::string::npos && i < str.length() && str[i])
    {
        i = str.find("//");
        if (i == std::string::npos)
            break ;
        str.erase(i, 1);
    }
    return (str);
}

bool    isdir( std::string filename )
{
    struct stat inf;
    const char *path;

    filename = leanLink(filename);
    path = filename.c_str();
    if (stat(path, &inf))
        return (false);
    if (S_ISDIR(inf.st_mode))
        return (true);
    return (false);
}

void    checkErrVal( int *err ) {

    if (*err == 0)
        *err = 200;
    else if (*err == -1)
        *err = 404;
    else
        *err = 500;
    return ;
}

int open_pipes( int *fds_0, int *fds_1 ) {

    if (pipe(fds_0) < 0 || pipe(fds_1) < 0)
        return (1);
    return (0);
}

void    close_pipes( int *fds_0, int *fds_1, int n ) {

    if (n != 1)
    {
        close(fds_0[1]);
        close(fds_1[0]);
    }
    if (n != 0)
    {
        close(fds_0[0]);
        close(fds_1[1]);
    }
}

void    Resp::setPip( void ) {

    int fod[6];

    do
    {
        this->pip.pipr = new struct pollfd[2];
        this->pip.pipw = new struct pollfd[2];
        this->pip.pipx = new int[2];
        if (!this->pip.pipr ||!this->pip.pipw ||!this->pip.pipx)
            break ;
        if (pipe(&fod[0]) < 0 ||pipe(&fod[2]) < 0 ||pipe(&fod[4]) < 0)
            break ;
        this->pip.pipr[0].fd = fod[0];
        this->pip.pipr[1].fd = fod[1];
        this->pip.pipw[0].fd = fod[2];
        this->pip.pipw[1].fd = fod[3];
        this->pip.pipx[0] = fod[4];
        this->pip.pipx[1] = fod[5];
        this->pip.pipr[0].events = POLLIN;
        this->pip.pipr[1].events = POLLOUT;
        this->pip.pipw[0].events = POLLIN;
        this->pip.pipw[1].events = POLLOUT;
        return ;
    } while (0);
    this->unsetPip();
    return ;
}

void    Resp::unsetPip( void ) {

    if (this->pip.pipr && fcntl(this->pip.pipr[0].fd, F_GETFD) != -1)
        close(this->pip.pipr[0].fd);
    if (this->pip.pipr && fcntl(this->pip.pipr[1].fd, F_GETFD) != -1)
        close(this->pip.pipr[1].fd);
    if (this->pip.pipw && fcntl(this->pip.pipw[0].fd, F_GETFD) != -1)
        close(this->pip.pipw[0].fd);
    if (this->pip.pipw && fcntl(this->pip.pipw[1].fd, F_GETFD) != -1)
        close(this->pip.pipw[1].fd);
    if (this->pip.pipx && fcntl(this->pip.pipx[0], F_GETFD) != -1)
        close(this->pip.pipx[0]);
    if (this->pip.pipx && fcntl(this->pip.pipx[1], F_GETFD) != -1)
        close(this->pip.pipx[1]);
    if (this->pip.pipr)
        delete [] this->pip.pipr;
    if (this->pip.pipw)
        delete [] this->pip.pipw;
    if (this->pip.pipx)
        delete [] this->pip.pipx;
    this->pip.pipr = NULL;
    this->pip.pipw = NULL;
    this->pip.pipx = NULL;
    return ;
}

struct pollfd   *Resp::getPipr( void ) {

    if (this->pip.mode == false)
        return (this->pip.pipr);
    return (NULL);
}

struct pollfd   *Resp::getPipw( void ) {

    if (this->pip.mode == true)
        return (this->pip.pipw);
    return (NULL);
}

struct pollfd   *Resp::STRICTgetPipw( void ) {

    return (this->pip.pipw);
}

bool    Resp::isCGI( void ) const {

    if (this->pip.pipr && this->pip.pipw)
        return (true);
    return (false);
}

bool    Resp::timeCGI( void ) {

    if (this->pip.tem.tv_sec == 0)
        gettimeofday(&this->pip.tem, NULL);
    else
    {
        if (get_timediff(this->pip.tem) >= 5)
            return (true);
    }
    return (false);
}

int Resp::launch_cgiscript( const Req *r ) {

    int err = 1;
    int arr[6] = {this->pip.pipr[0].fd, this->pip.pipr[1].fd, this->pip.pipw[0].fd, 
        this->pip.pipw[1].fd, this->pip.pipx[0], this->pip.pipx[1]};

    this->pip.mode = true;
    try {err = exec_cgiscript(r->getChosenLoc().cgi.front(), arr, *r);}
    catch (std::exception &e) {
         
        exit(-1);
    }
    if (err < 0)
        return (err);
    this->pip.pid = err; this->timeCGI(); return (err); }

void    Resp::readCGI( int fd ) {

    int len = 0;
    char    buff[CGI_READ_BUFFER];
    char    *tmp;

    memset(buff, 0, CGI_READ_BUFFER);
    len = read(fd, buff, CGI_READ_BUFFER - 1);
    if (len == -1);
    else if (len == 0)
        this->pip.mode = false;
    else
    {
        this->_binsize += len;
        tmp = new char[this->_binsize];
        if (!tmp)
            throw std::exception();
        buffncpy(tmp, this->_bin_data, this->_binsize - len);
        buffncpy(tmp + (this->_binsize - len), buff, len);
        if (this->_bin_data)
            delete [] this->_bin_data;
        this->_bin_data = tmp;
    }
    return ;
}

int Resp::reapChild( void ) {

    int status = 0;
    int ret = 0;

    ret = waitpid(this->pip.pid, &status, WNOHANG);
    if (ret == this->pip.pid && WIFEXITED(status))
        return (WEXITSTATUS(status));
    else if (ret == this->pip.pid && WIFSIGNALED(status))
        return (WTERMSIG(status));
    else
    {
        kill(this->pip.pid, SIGINT);
        waitpid(this->pip.pid, &status, 0);
    }
    return (-1);
}

bool    Resp::formatForTransm( const Req *r, int err, const t_server &serv  ) {

    char    *finn = NULL;
    std::string heds = "";
    std::string bd = "";



    if (err != 0)
    { 
        bd = getErrorBody(serv, 500);
        this->_binsize = bd.length();
        this->_status = 500;
        this->_header = new std::string(errorHead(500));
        heds = addCGI_headers(*r);
        delete this->_header;
        this->_header = NULL;
        heds.append(bd);
        this->setBuff(heds.c_str(), heds.length());
        this->setEnd(true);
    }
    else
    {
        if ( CUSTOMCGIHEAD && findRnrn(this->_bin_data, this->_binsize) != 0);
        else
        {
            this->_status = 200;
            this->_header = new std::string(errorHead(200));
            heds = addCGI_headers(*r);
            delete this->_header;
            this->_header = NULL;
            finn = new char[this->_binsize + heds.length()];
            if (!finn)
                return (false);
            buffncpy(finn, heds.c_str(), heds.length());
            buffncpy(finn + heds.length(), this->_bin_data, this->_binsize);
            this->setBuff(finn, this->_binsize + heds.length());
            delete [] finn;
        }
    }
    return (true);
}

std::string Resp::addCGI_headers( const Req &r ) {

    std::string message = "HTTP/1.1 ";

    message.append(*this->_header);
    message.append("\r\nServer: LgunnWebserv/v1");
    message.append("\r\n");
    if ((r.getConnection() == true && this->_status / 100 != 5))
        message.append("Connection: keep-alive");
    else
        message.append("Connection: close");
    message.append("\r\n" + this->getDateString());
    message.append("\r\nContent-Type: text/html");
    message.append("\r\nContent-Length: ");
    message.append(std::to_string(this->_binsize));
    message.append("\r\n\r\n");
    return (message);
}

template <typename T>
std::string *handleIndex( const Req &r, T empl, bool automode, int *indxerr ) {

    std::string     *ressfile;
    std::string     ressname;
    DIR             *dir_p;
    struct dirent   *ent_p;

    if (!automode && !empl.index.empty())
    {
        for (std::vector<std::string>::const_iterator it = empl.index.begin(); it != empl.index.end(); it ++)
        {
            ressname = leanLink(*it);
            if (RUN_AS_VERBOSE == true)
                std::cout << "Requested index ressource: " << ressname << std::endl;
            ressfile = const_cast<std::string *>(parseRess(ressname));
            if (!ressfile->empty())
            {
                *indxerr = 0;
                return (ressfile);
            }
            if (access(ressname.c_str(), F_OK))
                *indxerr = 404;
            else
                *indxerr = 403;
            delete ressfile;
        }
        if (empl.autoindex == -1)
            return (new std::string(""));
    }
    if (empl.autoindex)
    {
        dir_p = opendir(leanLink(r.getRessource()).c_str());
        ressfile = new std::string("");
        if (dir_p == NULL)
        {
            if (errno == EACCES)
                *indxerr = 403;
            else if (errno == ENOENT)
                *indxerr = 404;
            return (ressfile);
        }
        while (1)
        {
            ressname = "";
            ent_p = readdir(dir_p); 
            if (ent_p == NULL)
                break;
            if ((strlen(ent_p->d_name) == 1 && !strcmp(ent_p->d_name, "."))
                    ||(strlen(ent_p->d_name) == 2 && !strcmp(ent_p->d_name, "..")))
                continue ;
            ressfile->append("<a href=\"");
            ressname = r.getRequestedRessource();
            if (ressname.back() != '/')
                ressname += '/';
            ressname.append(ent_p->d_name);
            ressname = remove_excesSlash(ressname);
            ressfile->append(ressname);

            if (RUN_AS_VERBOSE == true)
                std::cout << "Ressfile: " << *ressfile << ", ressource: " << r.getRessource() << std::endl;
            ressfile->append("\">");
            ressfile->append(ent_p->d_name);
            ressfile->append("</a>");
            ressfile->append("<br>");
        }
        if (RUN_AS_VERBOSE == true)
            std::cout << *ressfile << std::endl;
        closedir(dir_p);
        *indxerr = 0;
        return (ressfile);
    }
    ressfile = new std::string("");
    return (ressfile);
}

std::string Resp::formatDateChars( int d, unsigned int zer) {

    std::string ret = std::to_string(d);
    

    while (ret.length() < zer)
        ret.insert(ret.begin(), '0');
    return (ret);
}

std::string Resp::getDateString( void ) {

    time_t  t = time(NULL);
    struct tm tm = *localtime(&t);
    std::string date_n_time("Date: ");
    std::string WDay[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    std::string MonN[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    

    date_n_time.append(WDay[tm.tm_wday] + ", " + this->formatDateChars(tm.tm_mday, 2));
    date_n_time.append(" " + MonN[tm.tm_mon] + " " + this->formatDateChars(tm.tm_year + 1900, 4) + " ");
    date_n_time.append(this->formatDateChars(tm.tm_hour, 2) + ":" + this->formatDateChars(tm.tm_min, 2) + ":");
    date_n_time.append(this->formatDateChars(tm.tm_sec, 2) + " GMT");
    return (date_n_time);
}

std::string Resp::add_headers( const Req &r, bool *close_flag, std::string &newloc ) {

    std::string message = "HTTP/1.1 ";

    message.append(*this->_header);
    message.append("\r\nServer: LgunnWebserv/v1");
    message.append("\r\n");
    if ((r.getConnection() == true && this->_status / 100 != 5))
    {
        *close_flag = true;
        message.append("Connection: keep-alive");
    }
    else
    {
        *close_flag = false;
        message.append("Connection: close");
    }
    message.append("\r\n" + this->getDateString());
    if (this->_bin_data || (this->_body && this->_body->length()))
    {
        if ( !r.getMethod().compare("GET") && GET_AS_DL == true && this->_status / 100 == 2)
        {
            message.append("\r\nContent-Type: application/octet-stream");
            message.append("\r\nContent-Disposition: attachment; filename=\"");
            message.append(r.getRequestedRessource().substr(r.getRequestedRessource().find("/")));
            message.append("\"");
        } else if (this->_status / 100 == 2)
            message.append("\r\nContent-Type: " + getContentType(r));
        else
            message.append("\r\nContent-Type: text/html");
        message.append("\r\nContent-Length: ");
        if (this->_bin_data)
            message.append(std::to_string(this->_binsize));
        else
            message.append(std::to_string(this->_body->length()));
    }
    else
        message.append("\r\nContent-Length: 0");
    if (this->_status / 100 == 3 || this->_status == 201)
    {
        if (newloc.empty())
            message.append("\r\nLocation: " + r.getRessource());
        else
            message.append("\r\nLocation: " + newloc);
    }
    message.append("\r\n\r\n");
    return (message);
}

void    Resp::reduceBuff(const char *s, int len, int written) {

    char    *nwbf;

    if (len - written <= 0)
        return ;
    nwbf = new char[len - written];
    if (!nwbf)
        return ;
    bzero(nwbf, len - written);
    buffncpy(nwbf, s + written, len - written);
    if (this->_bin_data)
        delete [] this->_bin_data;
    this->_bin_data = nwbf;
    this->_binsize = len - written;
    return ;
}

bool    isonlyzeros( const char *buff, int size) {

    for (int i = 0; i < size ; i ++)
    {
        if (buff[i])
            return (false);
    }
    return (true);
}

long    Resp::contsend( int fd ) {

    long ret = smartsend(fd, this->_bin_data, this->_binsize);

    return (ret);
}

void    Resp::unsetBuff( void ) {

    delete [] this->_bin_data;
    this->_bin_data = NULL;
    this->_binsize = 0;
}

void    Resp::setBuff( const char *s, unsigned int len ) {

    char *tmp;

    tmp = new char[len];
    if (!tmp)
        return ;
    buffncpy(tmp, s, len);
    this->_binsize = len;
    if (this->_bin_data)
        delete [] this->_bin_data;
    this->_bin_data = tmp;
    return ;
}

char    *Resp::getBuff( void ) {

    return (this->_bin_data);
}

unsigned int    Resp::getSize( void ) {

    return (this->_binsize);
}

long Resp::smartsend( int fd, const char *mess, unsigned int messlen ) {

    long    len = 0;
    
    if (getsockopt(fd, SOL_SOCKET, SO_TYPE, NULL, NULL) == -1 )
        len = write( fd, mess, messlen);
    else
        len = send( fd, mess, messlen, 0);
    if (len < 1)
        return (len);
    if (len < messlen)
    {
        this->saveFlag = true;
        this->reduceBuff(mess, messlen, len);
    }
    else
        this->saveFlag = false;
    return (len);
}

bool    Resp::transmit( int clifd, const Req &r, std::string &newloc ) {

    bool        close_flag = false;
    std::string message = add_headers(r, &close_flag, newloc);
    char        *binbf = NULL;
    int         len = 0;
    

    if (RUN_AS_VERBOSE == true)
        std::cout << "in transmit:" << std::endl << "ressource -> " << r.getRessource() << std::endl;
    if (this->_bin_data)
    {
        if (RUN_AS_VERBOSE == true)
            std::cout << std::endl << message << std::endl;
        binbf = new char[this->_binsize + message.length() + 1];
        if (!binbf)
            throw std::runtime_error("Couldn't allocate space for response");
        bzero(binbf, this->_binsize + message.length() + 1);
        buffncpy(binbf, message.c_str(), message.length());
        buffncpy(binbf + message.length(), this->_bin_data, this->_binsize);
        len = this->smartsend(clifd, binbf, this->_binsize + message.length());
        if (len == 0 || len == -1)
            close_flag = true;
        delete [] binbf;
    }
    else
    {
        if (this->_body)
            message.append(*this->_body);
        
        if (RUN_AS_VERBOSE == true)
            std::cout << std::endl << message << std::endl;
        len = this->smartsend(clifd, message.c_str(), message.length());
        if (len == 0 || len == -1)
            close_flag = true;
    }
    return (close_flag);
}

const std::string   errorHead( int err ) {

    std::map<int, std::string> respheader;

    respheader[100] = "100 Continue";
    respheader[200] = "200 OK";
    respheader[201] = "201 Created";
    respheader[204] = "204 No Content";
    respheader[303] = "303 See Other";
    respheader[400] = "400 Bad Request";
    respheader[403] = "403 Forbidden";
    respheader[404] = "404 Not Found";
    respheader[405] = "405 Method Not Allowed";
    respheader[408] = "408 Request Timeout";
    respheader[411] = "411 Length Required";
    respheader[413] = "413 Payload Too Large";
    respheader[414] = "414 URI Too Long";
    respheader[429] = "429 Too Many Requests";
    respheader[500] = "500 Internal Server Error";
    respheader[501] = "501 Not Implemented";
    respheader[503] = "503 Service Unavailable";
    respheader[505] = "505 HTTP Version Not Supported";
    return (respheader[err]);
}

const std::string   genErrorBody( int err ) {


    std::string ret = "<!DOCTYPE html><html><head><title>Error</title></head><body><h1>";
    std::string head = errorHead(err);

    if (head.empty())
        ret.append(errorHead(501));
    else
        ret.append(head);
    ret.append("</h1><hr><p>Could not serve content<p></body></html>");
    return (ret);
}

const std::string   getErrorBody( t_server serv, int err ) {

    std::string *ressfile;
    std::string ret;

    if (serv.error_pages.find(err) != serv.error_pages.end())
    {
        ressfile = const_cast<std::string *>(parseRess(serv.error_pages[err]));
        if (ressfile->empty())
        {   
            delete ressfile;
            return (genErrorBody(err));
        }
        ret.assign(*ressfile);
        delete ressfile;
        return (ret);
    }
    return (genErrorBody(err));
}

int Resp::binary_Get( const Req &r, const t_server &serv ) {

    std::ifstream   ifs(leanLink(r.getRessource()), std::ios::in | std::ios::binary | std::ios::ate);
    std::streampos  size;
    unsigned int    err = 0;

    if (!ifs.is_open())
    {
        if (access(leanLink(r.getRessource()).c_str(), F_OK))
            err = 404;
        else
            err = 500;
        this->_body = new std::string(getErrorBody(serv, err));
        return (err);
    }
    size = ifs.tellg();
    this->_binsize = size;
    this->_bin_data = new char[size];
    if (!this->_bin_data)
    {
        ifs.close();
        this->_body = new std::string(getErrorBody(serv, 500));
        return (500);
    }
    ifs.seekg (0, std::ios::beg);
    ifs.read(this->_bin_data, size);
    ifs.close();
    return (200);
}

int Resp::Get( const Req &r, const t_server serv ) {

    std::string *ressfile;
    int err = 500;
    int indxerr = 0;

    if ((r.getRequestedRessource().empty() ||!r.getRequestedRessource().compare("/"))
            && serv.index.empty() && serv.autoindex == 0 && !r.isChosen())
    {
        this->_body = new std::string(getErrorBody(serv, 404));
        return (404);
    }
    if (isdir(r.getRessource()))
    {
        if (r.isChosen())
        {
            if (r.getChosenLoc().index.empty() && r.getChosenLoc().autoindex == -1)
                this->_body = handleIndex(r, serv, true, &indxerr);
            else
            {
                this->_body = handleIndex(r, r.getChosenLoc(), false, &indxerr);
                if (r.getChosenLoc().autoindex == -1 && indxerr)
                {
                    delete this->_body;
                    this->_body = handleIndex(r, serv, true, &indxerr);
                }
            }
        }
        else
            this->_body = handleIndex(r, serv, false, &indxerr);
        if (indxerr)
        {
            if (this->_body)
                delete this->_body;
            this->_body = new std::string(getErrorBody(serv, indxerr));
            return (indxerr);
        }
        return (200);
    }
    if (getContentType(r).compare(0, 5, "text/"))
        return (this->binary_Get(r, serv));
    ressfile = const_cast<std::string *>(parseRess(leanLink(r.getRessource())));
    do
    {
        if (!ressfile->empty())
            break ;
        if (access(leanLink(r.getRessource()).c_str(), F_OK))
            err = 404;
        else if (access(leanLink(r.getRessource()).c_str(), R_OK))
            err = 403;
        else
            break ;
        this->_body = new std::string(getErrorBody(serv, err));
        delete ressfile;
        return (err);
    } while (0);
    this->_body = new std::string(*ressfile);
    delete ressfile;
    return (200);
}

int Resp::binary_Post( const Req &r, const t_server &serv, const std::string upldr ) {

    mode_t  md = CREATED_FILE_P;
    int fd = open(leanLink(upldr).c_str(), O_WRONLY | O_CREAT | O_EXCL, md);
    int status = 201;
    int ret = 0;

    if (RUN_AS_VERBOSE == true)
        std::cout << "Upldr is: " << upldr << ", fd is: " << fd << std::endl;
    if (fd < 0)
    {
        fd = open(leanLink(upldr).c_str(), O_WRONLY | O_TRUNC);
        if (fd > 1)
            status = 200;
        else
            return (500);
    }
    if (fd < 0)
    {
        if (access(upldr.c_str(), F_OK))
        {
            this->_body = new std::string(getErrorBody(serv, 404));
            return (404);
        }
        this->_body = new std::string(getErrorBody(serv, 403));
        return (403);
    }
    this->_body = new std::string("");
    ret = write(fd, r.getBody().data(), r.getBody().size());
    if (ret == 0 || ret == -1)
        status = 500;
    close(fd);
    return (status);
}

int Resp::Post( const Req &r, const t_server &serv, std::string &newloc ) {

    std::string     upldr;
    size_t          pos;

    if (r.getRequestedRessource().empty() ||!r.getRequestedRessource().compare("/"))
    {
        this->_body = new std::string(getErrorBody(serv, 404));
        return (404);
    }
    if (r.isChosen() && !r.getChosenLoc().upload_dir.empty())
    {
        pos = leanLink(r.getRessource()).rfind('/');
        if (!r.getChosenLoc().root.empty())
            upldr = r.getChosenLoc().root + r.getChosenLoc().upload_dir + leanLink(r.getRessource()).substr(pos);
        else
            upldr = serv.root + r.getChosenLoc().upload_dir + leanLink(r.getRessource()).substr(pos);
        newloc = remove_excesSlash(upldr);
    }
    else
        upldr = r.getRessource();
    if (RUN_AS_VERBOSE == true)
        std::cout << "Upload directory in POST: " << upldr << ", mode: " << r.isChosen() << !r.getChosenLoc().upload_dir.empty() << std::endl;
    if (!OVERWRITE_POST && (!access(leanLink(upldr).c_str(), F_OK) || isdir(upldr)))
    {
        this->_body = new std::string(getErrorBody(serv, 403));
        return (403);
    }
    else
        return (this->binary_Post(r, serv, upldr));
    return (201);
}

int Resp::Delete( const Req &r, const t_server &serv ) {

    unsigned int    err = 0;

    if (r.getRequestedRessource().empty() ||!r.getRequestedRessource().compare("/"))
        err = 404;
    else if (access(leanLink(r.getRessource()).c_str(), F_OK))
        err = 404;
    else if (access(leanLink(r.getRessource()).c_str(), W_OK))
        err = 403;
    else if (remove(leanLink(r.getRessource()).c_str()))
        err = 500;
    if (err != 0)
    {
        this->_body = new std::string(getErrorBody(serv, err));
        return (err);
    }
    else
        this->_body = new std::string("");
    return (204);
}

int Resp::handle_reqlocproperties( const Req &r ) {

    int status = 0;

    if (r.isChosen() && !r.getChosenLoc().redirect.empty())
    {
        const_cast<Req &>(r).setRessource(r.getChosenLoc().redirect);
        status = 303;
    }
    else if (r.getMethod().compare("DELETE") && r.isChosen()
        && !r.getChosenLoc().cgi.empty())
    {
        this->setPip();
        this->saveFlag = true;
        status = 555;
    }
    return (status);
}

void    Resp::methodHandler( const Req &r, const t_server serv, std::string &newloc ) {

    int status;
    std::string buff;

    status = handle_reqlocproperties(r);
    if (status == 555)
        return ;
    else if (status != 0);
    else if (!r.getMethod().compare("GET"))
        status = this->Get(r, serv);
    else if (!r.getMethod().compare("POST"))
        status = this->Post(r, serv, newloc);
    else if (!r.getMethod().compare("DELETE"))
        status = this->Delete(r, serv);
    this->_header = new std::string(errorHead(status));
    this->_status = status;
    return ;
}

std::string Resp::getContentType( const Req &r ) {

    std::string extn = r.getRessource().substr(r.getRessource().rfind('.') + 1);
    std::map<std::string, std::string>  valtypesmap;

    valtypesmap.insert(std::pair<std::string, std::string>("html", "text/html"));
    valtypesmap.insert(std::pair<std::string, std::string>("css", "text/css"));
    valtypesmap.insert(std::pair<std::string, std::string>("csv", "text/csv"));
    valtypesmap.insert(std::pair<std::string, std::string>("js", "text/javascript"));
    valtypesmap.insert(std::pair<std::string, std::string>("txt", "text/plain"));
    valtypesmap.insert(std::pair<std::string, std::string>("xml", "text/xml"));
    valtypesmap.insert(std::pair<std::string, std::string>("gif", "image/gif"));
    valtypesmap.insert(std::pair<std::string, std::string>("jpg", "image/jpeg"));
    valtypesmap.insert(std::pair<std::string, std::string>("png", "image/png"));
    valtypesmap.insert(std::pair<std::string, std::string>("mp3", "audio/mp3"));
    valtypesmap.insert(std::pair<std::string, std::string>("mp4", "video/mp4"));
    std::string valtypes[11] = {"html", "css", "csv", "js", "txt", "xml", "gif", "jpg", "png", "mp3", "mp4"};

    for (int i = 0; i < 11; i ++)
    {
        if (!extn.compare(valtypes[i]))
            return (valtypesmap[valtypes[i]]);
    }
    return ("text/html");
}

void    Resp::setEnd( bool b ) {

    this->endco = b;
    return ;
}

bool  Resp::endItNow( void ) const {

    return (this->endco);
}

void    Resp::setSaved( bool b ) {

    this->saveFlag = b;
    return ;
}

bool  Resp::isSaved( void ) const {

    return (this->saveFlag);
}

Resp::Resp( void ) {

    return ;
}

Resp::Resp( const Resp & ) {

    return ;
}

Resp::Resp( const Req &r, const t_server &serv, std::string &newloc ) {


    this->_header = NULL;
    this->_body = NULL;
    this->_status = 0;
    this->_bin_data = NULL;
    this->pip.pipr = NULL;
    this->pip.pipw = NULL;
    this->pip.pipx = NULL;
    this->pip.mode = false;
    this->pip.pid = 0;
    this->pip.tem.tv_sec = 0;
    this->pip.tem.tv_usec = 0;
    this->endco = false;
    this->saveFlag = false;
    this->methodHandler(r, serv, newloc);

    return ;
}

Resp::~Resp( void ) { 

    if (this->_header)
    {
        delete this->_header;
        this->_header = NULL;
    }
    if (this->_body)
    {
        delete this->_body;
        this->_body = NULL;
    }
    if (this->_bin_data)
    {
        delete [] this->_bin_data;
        this->_bin_data = NULL;
    }
    return ;
}

const Resp &Resp::operator=( const Resp &inst ) {

    return (inst);
}
