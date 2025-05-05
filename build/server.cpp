#include "webserv.hpp"


void    startserver( std::vector<t_server> serv )
{
    std::vector<ServInstance>   instances;
    bool                        inlist_flag = false;

    instances.push_back(serv.front());
    for (std::vector<t_server>::const_iterator it = serv.begin(); it != serv.end(); it ++)
    {
        try
        { 
            inlist_flag = false;
            for (std::vector<ServInstance>::iterator ct = instances.begin(); ct != instances.end(); ct ++)
            { if (ct->hservs.empty() || ct->hservs.front().port == it->port)
                {
                    ct->hservs.push_back(*it);
                    inlist_flag = true;
                    break ;
                }
            }
            if (inlist_flag == false) 
            {
                instances.push_back(ServInstance(*it));
                instances.back().hservs.push_back(*it);
            }
        } catch (std::runtime_error &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return ;
        }
    }
    server_loop(instances, serv);
    for (std::vector<ServInstance>::iterator mt = instances.begin(); mt != instances.end(); mt ++)
    {
        for (std::vector<struct pollfd>::iterator qt = mt->fds.begin(); qt != mt->fds.end(); qt ++)
            close(qt->fd);

    }
    return ;
}

void    selectServerInst( std::vector<char> &tmp, const ServInstance &inst, t_server *serv_selector ) {

    std::string link;
    char        *bf;
    char        *fdr;

    if (tmp.empty())
        return ;
    bf = strnstr(tmp.data(), "Host: ", tmp.size());
    if (!bf)
        return ;
    fdr = strnstr(bf, "\r\n", tmp.size() - (bf - tmp.data()));
    if (!fdr)
        return ;
    bf += 6;
    link.assign(bf, fdr - bf);
    for (std::vector<t_server>::const_iterator it = inst.hservs.begin(); it != inst.hservs.end(); it ++)
    {
        for (std::vector<std::string>::const_iterator ct = it->server_names.begin(); ct != it->server_names.end(); ct ++)
        {
            if ((link.find(":") != std::string::npos && !ct->compare(link.substr(0, link.find(":"))))
                ||(link.find(":") == std::string::npos && !ct->compare(link)))
            {
                *serv_selector = *it;
                return ;
            }       
        }
    }
    *serv_selector = inst.hservs.front();
    return ;
}

std::string get_strfrombuff( const std::vector<char> &bin_buff ) {

    std::string  ret = "";
    char *tmp = new char[bin_buff.size() + 1];

    bzero(tmp, bin_buff.size());
    strncpy(tmp, bin_buff.data(), bin_buff.size()) ;
    if (!(bin_buff.size() == 0))
        ret.append(tmp, bin_buff.size());
    delete [] tmp;
    return (ret);
}

void    buffncpy( char *dst, const char *src, int len )
{
    if (!src || !dst)
        return ;
    for (int i = 0; i < len; i ++)
        dst[i] = src[i];
    return ;
}

void    add_bufftoback( std::vector<char> &bin_buff, char *bf, int size ) {

    int len = bin_buff.size();

    if (!bin_buff.data())
    {
        bin_buff.reserve(size);
        bin_buff.resize(size);
        bzero(bin_buff.data(), size);
    }
    else
        bin_buff.resize(size + len);
    buffncpy(&bin_buff.data()[len], bf, size);
    return ;
}

void   end_con( t_client &cli, ServInstance &inst, int i ) {
    
    close(inst.fds[i + 1].fd);
    if (cli.bin_buffers.size() > 0)
        cli.bin_buffers.erase(cli.bin_buffers.begin() + i);
    if (inst.fds.size() > 1)
        inst.fds.erase(inst.fds.begin() + i + 1);
    if (cli.last_mod.size() > 0)
        cli.last_mod.erase(cli.last_mod.begin() + i);
    if (cli.CRLFpos.size() > 0)
        cli.CRLFpos.erase(cli.CRLFpos.begin() + i);
    return ;
}

void    quick_resp( int err, int fd, t_server &serv, bool *end_flag ) {

    std::string message = "HTTP/1.1 ";
    std::string errbd;
    int len = 0;

    message.append(errorHead(err));
    message.append("\r\nServer: LgunnWebserv/v1");
    if (err / 100 == 4 || err / 100 == 5)
    {
        errbd = getErrorBody(serv, err);
        if (err == 408  || err == 413 || err == 400 || err / 100 == 5)
            message.append("\r\nConnection: close");
        else
            message.append("\r\nConnection: keep-alive");
        message.append("\r\nContent-Type: text/html");
        message.append("\r\nContent-Length: ");
        message.append(std::to_string(errbd.length()));
    }
    else
        message.append("\r\nConnection: keep-alive");
    message.append("\r\n\r\n");
    message.append(errbd);
    if (RUN_AS_VERBOSE == true)
        std::cout << std::endl << message << std::endl;
    len = send(fd, message.c_str(), message.length(), 0);
    if ((len == -1 ||len == 0) && end_flag != NULL)
        *end_flag = true;
    return ;
}

void    switchfds(struct pollfd *fd1, struct pollfd *fd2) {

    int tmp = fd1->fd;
    short evs = fd1->events;
    short revs = fd1->revents;


    fd1->fd = fd2->fd;
    fd1->events = fd2->events;
    fd1->revents = fd2->revents;
    fd2->fd = tmp;
    fd2->events = evs;
    fd2->revents = revs;
    return ;
}

void    handle_Request( t_client &cli, ServInstance &inst, int i ) {

    Req         *requestptr;
    Resp        *responseptr;
    t_server    *serv_selector;
    long        ret = 0;
    std::string newloc = "";

    serv_selector = new t_server(inst.hservs.front());
    selectServerInst(cli.bin_buffers[i] , inst, serv_selector);
    try
    {
        if (!cli.savedReq[i] && !cli.savedResp[i])
        {
            requestptr = new Req(cli.bin_buffers[i], *serv_selector);
            responseptr = new Resp(*requestptr, *serv_selector, newloc);
            if (responseptr->getPipr())
            {
                responseptr->setBuff(cli.bin_buffers[i].data(), cli.bin_buffers[i].size());
                switchfds(&inst.fds[i + 1], &responseptr->getPipr()[1]);
            }
            else if (responseptr->transmit(inst.fds[i + 1].fd, *requestptr, newloc) == true)
                responseptr->setEnd(true);
        }
        else if (cli.savedResp[i]->getPipr())
        {
            responseptr = cli.savedResp[i];
            requestptr = cli.savedReq[i];
            ret = cli.savedResp[i]->contsend(inst.fds[i + 1].fd);
            if (!responseptr->isSaved())
                responseptr->setSaved(true);
            if (ret != 0 && ret != -1)
            {
                if (cli.savedResp[i]->getSize() <= 0 ||ret >= cli.savedResp[i]->getSize())
                {
                    cli.savedResp[i]->unsetBuff();
                    switchfds(&inst.fds[i + 1], &cli.savedResp[i]->getPipr()[1]);
                    if (cli.savedResp[i]->launch_cgiscript(cli.savedReq[i]) < 0)
                    {
                        cli.savedResp[i]->unsetPip();
                        cli.savedResp[i]->formatForTransm(cli.savedReq[i], -12, *serv_selector);
                        return ;
                    }
                    close(cli.savedResp[i]->getPipw()[1].fd);
                    switchfds(&inst.fds[i + 1], &cli.savedResp[i]->getPipw()[0]);
                }
            }
        }
        else if (cli.savedResp[i]->getPipw())
            return ;
        else
        {
            ret = cli.savedResp[i]->contsend(inst.fds[i + 1].fd);
            if (cli.savedResp[i]->getSize() <= 0 || ret == 0 || ret == -1)
                end_con(cli, inst, i);
            responseptr = cli.savedResp[i];
            requestptr = cli.savedReq[i];
        }


        if (cli.savedResp[i] && cli.savedResp[i]->getSize() <= 0 && cli.savedResp[i]->endItNow())
            end_con(cli, inst, i);
        if (responseptr && responseptr->isSaved())
        {
            if (cli.savedReq[i])
                return ;
            cli.savedReq[i] = requestptr;
            cli.savedResp[i] = responseptr;
            return ;
        }
        else if (cli.savedReq[i] && cli.savedResp[i])
        {
            cli.savedReq[i] = NULL;
            cli.savedResp[i] = NULL;
        }

        if (responseptr)
            delete responseptr;
        if (requestptr)
            delete requestptr;
    }
    catch (request_exception &r) {
        handle_connectionerror(r.getErr(), cli, inst, i);
        if (RUN_AS_VERBOSE == true)
            std::cerr << "Error When Processing Request: " << errorHead(r.getErr()) << std::endl;
}
catch (std::exception &e)
{
    quick_resp(500, inst.fds[i].fd, *serv_selector, NULL);
    end_con(cli, inst, i);
}


delete serv_selector;
return ;
}

long    get_ContentLength( std::string &text ) {

    unsigned long   pos;
    long            len = -1;
    
    pos = text.find("Content-Length:");
    if (pos != std::string::npos)
    {
        pos +=  15;
        try {len = std::stoi(text.substr(pos, text.find("\r\n", pos) - pos));}
        catch (std::exception &e) {return (len);}
    }
    else if (pos == std::string::npos && text.length() > MAX_HEAD_SIZE)
        len = -2;
    else
        len = -1;
    return (len);
}

long    get_ContentLength( std::vector<char> &text ) {

    char            *bf;
    char            *nd;
    char            *tmp;
    long            len = -1;


    bf = strnstr(text.data(), "Content-Length: ", text.size());
    if (!bf)
        return (-1);
    nd = strnstr(bf, "\r\n", text.size() - (bf - text.data()) );
    if (bf && nd)
    {
        len = nd - bf - 16;
        tmp = new char[len + 1];
        bzero(tmp, len + 1);
        strncpy(tmp, bf + 16, len);
        len = atol(tmp);
        delete [] tmp;
    }
    else if (text.size() > MAX_HEAD_SIZE)
        len = -2;
    else
        len = -1;
    return (len);
}

long    get_ContentLength( const char *text ) {

    const char      *bf = strnstr(text, "Content-Length: ", strlen(text));
    if (!bf)
        return (-1);
    const char      *nd = strnstr(bf, "\r\n", strlen(text) - (bf - text));
    char            *tmp;
    long            len = -1;
    
    if (bf && nd)
    {
        len = nd - bf - 16;
        tmp = new char[len + 1];
        bzero(tmp, len + 1);
        strncpy(tmp, bf + 16, len);
        len = atol(tmp);
        delete [] tmp;
    }
    else
        len = -1;
    return (len);
}

bool    isFullRequest( std::vector<char> &bin_buff) {

unsigned long   pos;
std::string     buff = get_strfrombuff(bin_buff);

pos = buff.find("\r\n\r\n");
if (pos != std::string::npos && (long)(bin_buff.size() - pos) >= get_ContentLength(buff))
    return (true);
return (false);
}

bool    complete_req( t_client &cli, ServInstance &inst, int i ) {

    std::string bf = get_strfrombuff(cli.bin_buffers[i]);
    long        len;
    
    if (!get_methodfrombuff(bf).compare("POST") && get_bodyfrombuff(bf).empty())
    {
        if (cli.readReady[i] && bf.find("Expect: 100-continue") != std::string::npos)
        {
            handle_connectionerror(100, cli, inst, i);
            len = get_ContentLength(bf);
            if (len >= 0)
                cli.bin_buffers[i].reserve(len);
            else
                return (false);
            cli.readReady[i] = false;
            inst.prio = true;
        }
        return (true);
    }
    return (false);
}

void    cycle_writeFds( t_client &cli, ServInstance &inst ) {

    int     i = 0;

    for (std::vector<struct pollfd>::const_iterator it = inst.fds.begin() + 1; it != inst.fds.end(); it ++)
    {
        if (!cli.oppskip[i])
        {
            if ((it->revents & POLLOUT) && (isFullRequest(cli.bin_buffers[i]) ||cli.savedResp[i]))
            {
                handle_Request(cli, inst, i);
                if (inst.fds.size() < 2) 
                    return ;
                if (cli.bin_buffers[i].size() > 0)
                    cli.bin_buffers[i].clear();
            }
            else if ((it->revents & POLLOUT) && !isFullRequest(cli.bin_buffers[i]))
                complete_req(cli, inst, i);
        }
        else
            cli.oppskip[i] = false;
        i ++;
    }
    (void)inst;
    return ;
}

void    handle_connectionerror( int err, t_client &cli, ServInstance &inst, int i) {

    t_server    *serv_selector;
    bool        end_flag = false;

    serv_selector = new t_server(inst.hservs.front());
    selectServerInst(cli.bin_buffers[i] , inst, serv_selector);
    quick_resp(err, inst.fds[i + 1].fd, *serv_selector, &end_flag);
    if (err == 408 || err == 413  || err == 400 || err / 100 == 5 || end_flag == true)
        end_con(cli, inst, i);
    delete serv_selector;
    return ;
}

long    find_crnl( std::vector<char> &buff ) {

   char *p = strstr(buff.data(), "\r\n\r\n");
   
   if (!p)
       return (-1);
   return (p - buff.data());
}

void    check_stateofbuffer( t_client &cli, ServInstance &inst, int i) {

    unsigned long   cl;
    unsigned long   serv_cli_bd_l;
    t_server    *serv_selector;


    if (cli.CRLFpos[i] == -1 && find_crnl(cli.bin_buffers[i]) >= 0)
    {
        gettimeofday(&cli.last_mod[i], NULL);
        cli.CRLFpos[i] = find_crnl(cli.bin_buffers[i]);
        if (cli.CRLFpos[i] > MAX_HEAD_SIZE)
            handle_connectionerror( 413, cli, inst, i);
    }
    gettimeofday(&cli.last_mod[i], NULL);
    serv_selector = new t_server(inst.hservs.front());
    selectServerInst(cli.bin_buffers[i], inst, serv_selector);
    cl = get_ContentLength(cli.bin_buffers[i]);
    serv_cli_bd_l = serv_selector->cli_body_len;
    delete serv_selector;
    if (cl == std::string::npos || cl < 0)
        return ;
    if (cl > 0 && cl > serv_cli_bd_l)
        handle_connectionerror( 413, cli, inst, i);
    else if (/*(strlen(cli.bin_buffers[i].data())*/cli.bin_buffers[i].size() - cli.CRLFpos[i] - 4 > cl)
        handle_connectionerror( 400, cli, inst, i);
    (void)inst;
    return ;
}

int get_dynBuffSize( void )
{
    int n = 1024;

    while (n < DEF_MAX_BODY)
    {
        if (n > MAX_BIN_MULT)
            break ;
        n *= 2;
    }
    return (n);
}

void    cycle_readFds( t_client &cli, ServInstance &inst ) {

    std::vector<struct pollfd> tmp = inst.fds;
    t_server *serv_selector = NULL;
    int         sz = get_dynBuffSize();
    char        *buff = new char[sz];
    int         i = 0;
    int         err = 0;
    

    sz -= 1;
    for (std::vector<struct pollfd>::const_iterator it = tmp.begin() + 1; i < static_cast<int>(inst.fds.size() - 1) && it != tmp.end(); it ++)
    {
        if (it->revents & POLLIN)
        {
            cli.oppskip[i] = true;
            bzero(buff, sz + 1);

            if (!isSocket(it->fd))
            {
                try
                {
                    cli.savedResp[i]->readCGI(inst.fds[i + 1].fd);
                } catch (std::exception &e) {

                    switchfds(&inst.fds[i + 1], &cli.savedResp[i]->getPipr()[0]);
                    cli.savedResp[i]->unsetPip();
                    handle_connectionerror( 500, cli, inst, i);
                    return ;
                }
                if (cli.savedResp[i]->getPipr() ||cli.savedResp[i]->timeCGI())
                {
                    switchfds(&inst.fds[i + 1], &cli.savedResp[i]->STRICTgetPipw()[0]);
                    cli.savedResp[i]->unsetPip();
                    serv_selector = new t_server(inst.hservs.front());
                    selectServerInst(cli.bin_buffers[i] , inst, serv_selector);
                    cli.savedResp[i]->formatForTransm(cli.savedReq[i], cli.savedResp[i]->reapChild(), *serv_selector);
                    delete serv_selector;
                    cli.savedResp[i]->unsetPip();
                }
                continue ;
            }
            errno = 0;
            err = recv(it->fd, buff, sz, MSG_DONTWAIT);
            if (err == -1 || err == 0)
            {
                inst.prio = false;
                end_con(cli, inst, i);
            }
            else
            {
                if (err < sz && !cli.readReady[i])
                {
                    cli.readReady[i] = true;
                }
                add_bufftoback(cli.bin_buffers[i], buff, err);
                check_stateofbuffer(cli, inst, i);
            }
        }
        else
        {
            cli.oppskip[i] = false;
            if (inst.prio)
                inst.prio = false;
        }
        i ++;
    }
    delete [] buff;
    return ;
}

void    check_IncomingConnections( ServInstance &inst, t_client &cli) {

    struct sockaddr_in  cliaddr;
    struct pollfd       tmp;
    struct timeval      tmp2;
    std::vector<char>   tmp3;
    long                tmp4 = -1;
    bool                tmp5 = false;
    bool                tmp6 = false;
    socklen_t           len;

    bzero(&cliaddr, sizeof(cliaddr));
    len = sizeof(cliaddr);
    tmp.fd = accept(inst.fds[0].fd, (struct sockaddr *)&cliaddr, &len);
    if (tmp.fd != -1)
    {
        //if (inst.fds.size() >= 8000)  // do we keep this???
        //{
            //close(inst.fds.back().fd);
            //inst.fds.erase(inst.fds.begin());
                                                    //// t'as oas fermè mongole !!!
            //cli.bin_buffers.erase(cli.bin_buffers.begin());
            //cli.last_mod.erase(cli.last_mod.begin());
            //cli.CRLFpos.erase(cli.CRLFpos.begin());
        //}
        // j'enlüve un moment, j'aimerai bien m'en débaraser
        tmp.events = POLLIN | POLLOUT;
        inst.fds.push_back(tmp);
        cli.bin_buffers.push_back(tmp3);
        gettimeofday(&tmp2, NULL);
        cli.last_mod.push_back(tmp2);
        cli.CRLFpos.push_back(tmp4);
        cli.readReady.push_back(tmp5);
        cli.oppskip.push_back(tmp6);
        cli.savedReq.push_back(NULL);
        cli.savedResp.push_back(NULL);
    }
    return ;
}

unsigned int    get_timediff( struct timeval last ) {

    struct timeval  tm;

    gettimeofday(&tm, NULL);
    return (tm.tv_sec - last.tv_sec);
}

void    check_timeout( t_client &cli, ServInstance &inst ) {

    unsigned long   i = 0;

    for (std::vector<struct timeval>::const_iterator it = cli.last_mod.begin(); it != cli.last_mod.end() && i < cli.last_mod.size(); it ++)
    {
        if (get_timediff(*it) > TIMEOUT_VAL)
            handle_connectionerror( 408, cli, inst, i);
        if (cli.savedResp[i] && cli.savedResp[i]->isCGI() 
                && cli.savedResp[i]->timeCGI())
        {
            cli.savedResp[i]->reapChild();
            switchfds(&inst.fds[i + 1], &cli.savedResp[i]->getPipw()[0]);
            cli.savedResp[i]->unsetPip();
            cli.savedResp[i] = NULL;
            cli.savedReq[i] = NULL;
            handle_connectionerror( 500, cli, inst, i);
        }

        i ++;
    }
    return ;
}

void    server_loop( std::vector<ServInstance> &instances, std::vector<t_server> serv )
{
    int ready_flag = 0;

    while (SIG_HANDLER_GLOBAL == 0)
    {
        for (std::vector<ServInstance>::iterator it = instances.begin(); it != instances.end(); it ++)
        {
            check_timeout(it->cli, *it);
            ready_flag = poll(it->fds.data(), it->fds.size(), 100);
            if (ready_flag < 0)
            {
                if (SIG_HANDLER_GLOBAL != 0)
                    break ;
                else
                    throw std::runtime_error("Polling failed");
            }
            if (it->fds[0].revents & POLLIN)
                check_IncomingConnections(*it, it->cli);
            if (it->fds.size() > 1)
            {
                cycle_readFds(it->cli, *it);
                cycle_writeFds(it->cli, *it);
            }
        }
    }
    (void)serv;
    return ;
}
