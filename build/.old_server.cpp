
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
            {
                if (ct->hservs.empty() || ct->hservs.front().port == it->port)
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
        close(mt->sockfd);
    return ;
}

void    selectServerInst( std::string tmp, const ServInstance &inst, t_server *serv_selector ) {

    std::string link;

    if (tmp.find("Host: ") == std::string::npos)
        return ;
    link = tmp.substr(tmp.find("Host: ") + 6, tmp.find("\r\n", tmp.find("Host: ")) - (tmp.find("Host: ") + 6));
    for (std::vector<t_server>::const_iterator it = inst.hservs.begin(); it != inst.hservs.end(); it ++)
    {
        for (std::vector<std::string>::const_iterator ct = it->server_names.begin(); ct != it->server_names.end(); ct ++)
        {
            if (!ct->compare(link))
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

void    add_bufftoback( std::vector<char> &bin_buff, char *bf, int size ) {

    for (int i = 0; i < size; i ++) // binary buffer is probably slow
    {
        bin_buff.push_back(bf[i]);
    }
    return ;
}

void    end_con( t_client &cli, int i ) {
    
    if (i > 0)
        i -= 1;
    close(cli.fds[i].fd);
    if (cli.bin_buffers.size() > 0)
        cli.bin_buffers.erase(cli.bin_buffers.begin() + i);
    if (cli.fds.size() > 0)
        cli.fds.erase(cli.fds.begin() + i);
    if (cli.last_mod.size() > 0)
        cli.last_mod.erase(cli.last_mod.begin() + i);
    if (cli.CRLFpos.size() > 0)
        cli.CRLFpos.erase(cli.CRLFpos.begin() + i);
    return ;
}

void    quick_resp( int err, int fd, t_server serv ) {

    std::string message = "HTTP/1.1 ";
    std::string errbd;

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
    //std::cerr << "Message:" << std::endl << message << std::endl;
    send(fd, message.c_str(), message.length(), 0); // faut check la valeur d'err
    return ;
}


void    handle_Request( t_client &cli, ServInstance &inst, int i ) {

    Req         *requestptr;
    Resp        *responseptr;
    t_server    *serv_selector;
    std::string newloc = "";

    serv_selector = new t_server(inst.hservs.front());
    selectServerInst(get_strfrombuff(cli.bin_buffers[i]) , inst, serv_selector);
    try
    {
        requestptr = new Req(cli.bin_buffers[i], *serv_selector);
        responseptr = new Resp(*requestptr, *serv_selector, newloc);
        if (responseptr->transmit(cli.fds[i].fd, *requestptr, newloc) == false)
            end_con(cli, i);
        delete responseptr;
        delete requestptr;
    }
    catch (request_exception &r) {
        handle_connectionerror(r.getErr(), cli, inst, i);
        if (RUN_AS_VERBOSE == true)
            std::cerr << "Fatal Error in Request/Response cycle: " << errorHead(r.getErr()) << std::endl;
    }
    catch (std::exception &e)
    {
        std::cerr << "Fatal! " << e.what() << std::endl;
        quick_resp(500, cli.fds[i].fd, *serv_selector);
        end_con(cli, i);
    }
    delete serv_selector;
    return ;
}


long    get_ContentLength( std::string text ) {

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

bool    isFullRequest( std::vector<char> &bin_buff) {

    unsigned long   pos;
    std::string     buff = get_strfrombuff(bin_buff);

    pos = buff.find("\r\n\r\n");
    if (pos != std::string::npos && (long)(bin_buff.size() - pos) >= get_ContentLength(buff))
        return (true);
    return (false);
}
// handle expect 100?
// how do we implement this exactly??
//  - when we read expect 100 in header (hard code blk)
//      - we send 100, we need to write to somewhere, named pipe?
//      - or we just save it in the buffer, but then it could get really big?

bool    complete_req( t_client &cli, ServInstance &inst, int i ) {

    std::string bf = get_strfrombuff(cli.bin_buffers[i]);

    if (!get_methodfrombuff(bf).compare("POST") && get_bodyfrombuff(bf).empty())
    {
        //std::cerr << "=====" << std::endl << "Body" << std::endl << "=====" << std::endl;
        //std::cerr << get_bodyfrombuff(bf) << std::endl;
        //std::cerr << bf << std::endl;
        //std::cerr << "With len: " << bf.length() << std::endl;
        //std::cerr << "=====" << std::endl << "EOBOBBY" << std::endl << "=====" << std::endl;
        if (cli.readReady[i])
        {
            handle_connectionerror(100, cli, inst, i);
            cli.readReady[i] = false;
        }
        return (true);
    }
    return (false);
}

void    cycle_writeFds( t_client &cli, ServInstance &inst ) {

    int     i = 0;
    std::vector<struct pollfd> tmp = cli.fds;
    std::vector<std::vector<char> > tmp2 = cli.bin_buffers;

    for (std::vector<struct pollfd>::const_iterator it = tmp.begin(); it != tmp.end(); it ++)
    {
        if ((it->revents & POLLOUT) && isFullRequest(tmp2[i]))
        {
            handle_Request(cli, inst, i);
            cli.bin_buffers[i].clear();
        }
        else if ((it->revents & POLLOUT) && !isFullRequest(tmp2[i]))
            complete_req(cli, inst, i); // this only needs to happen once 
        i ++;                           // doesn't matter, it's ok like this
    }                                   // maybe check also that there's the expect l
    (void)inst;
    return ;
}

void    handle_connectionerror( int err, t_client &cli, ServInstance &inst, int i) {

    t_server    *serv_selector;

    serv_selector = new t_server(inst.hservs.front());
    selectServerInst(get_strfrombuff(cli.bin_buffers[i]) , inst, serv_selector);
    quick_resp(err, cli.fds[i].fd, *serv_selector);
    if (err == 408 || err == 413  || err == 400 || err / 100 == 5)
        end_con(cli, i);
    delete serv_selector;
    return ;
}

void    check_stateofbuffer( t_client &cli, ServInstance &inst, int i) {

    unsigned long   cl;
    unsigned long   serv_cli_bd_l;
    std::string tmp = get_strfrombuff(cli.bin_buffers[i]);
    t_server    *serv_selector;


    if (cli.CRLFpos[i] == -1 && tmp.rfind("\r\n\r\n") != std::string::npos)
    {
        gettimeofday(&cli.last_mod[i], NULL);
        cli.CRLFpos[i] = get_strfrombuff(cli.bin_buffers[i]).find("\r\n\r\n");
        if (cli.CRLFpos[i] > MAX_HEAD_SIZE) // ?
        {
            handle_connectionerror( 413, cli, inst, i); // ?
        }
    }
    else if (cli.CRLFpos[i] != -1)
    {
        gettimeofday(&cli.last_mod[i], NULL);
        serv_selector = new t_server(inst.hservs.front());
        selectServerInst(tmp, inst, serv_selector);
        cl = get_ContentLength(tmp);
        serv_cli_bd_l = serv_selector->cli_body_len;
        delete serv_selector;
        if (cl == std::string::npos || cl < 0)
            return ;
        if (cl > 0 && cl > serv_cli_bd_l)
        {
            handle_connectionerror( 413, cli, inst, i);
        }
        else if ((cli.bin_buffers[i].size() - cli.CRLFpos[i]) - 4 > cl)
            handle_connectionerror( 400, cli, inst, i);
    }
    (void)inst;
    return ;
}

void    cycle_readFds( t_client &cli, ServInstance &inst ) {

    std::vector<struct pollfd> tmp = cli.fds;
    //char        buff[50000001];
    char        *buff = new char[5000001]; // faut trv la bonne taille pour ça...
    int         i = 0;
    int         err = 0;

    //bzero(buff, sizeof(buff));
    //bzero(buff, 500000001);
    for (std::vector<struct pollfd>::const_iterator it = tmp.begin(); i < static_cast<int>(cli.fds.size()) && it != tmp.end(); it ++)
    {
        if (it->revents & POLLIN)
        {
            //std::cerr << "read ready" << std::endl;
            bzero(buff, 5000001);
            //bzero(buff, sizeof(buff));
            err = recv(it->fd, buff, 5000000, MSG_DONTWAIT);
            if (err == -1 || err == 0)
            {
                inst.prio = false;
                std::cerr << "ENDING CONNECTION!!!!!!!!!!!" << std::endl;
                end_con(cli, i);
            }
            else
            { // should only say continue again when it has read everything...
                //std::cerr << "Bytes read: " << err << std::endl;
                if (err < 5000000 && !cli.readReady[i]) /// neeeeeeeew
                {
                    //inst.prio = false;
                    cli.readReady[i] = true;
                }
                cli.bin_buffers[i].reserve(cli.bin_buffers[i].size() +  err);
                //cli.bin_buffers[i].assign(buff, buff + err - 1); // assigns, does not add to back
                add_bufftoback(cli.bin_buffers[i], buff, err);
                //check_stateofbuffer(cli, inst, i);      // add this again after
                if (cli.bin_buffers[i].size() > cli.perce * 5000000)
                {
                    std::cerr << "yess";
                    std::cerr << "*";
                    cli.perce ++;
                }
                std::cerr << cli.bin_buffers[i].size() << "  ";
                //std::cerr << "For: " << i << ": " << cli.bin_buffers[i].size() << std::endl;
            }
        }
        i ++;
    }
    delete [] buff;
    return ;
} // il ralentit psk il doit trimbaler tout ici... ça serait plus rapide de retourner un tableau qui se fait copier
  // comme ça on déplace juste ce qui change... / ce dont on a besoin

void    check_IncomingConnections( int sockfd, t_client &cli) {

    struct sockaddr_in  cliaddr;
    struct pollfd       tmp;
    struct timeval      tmp2;
    std::vector<char>   tmp3;
    long                tmp4 = -1;
    bool                tmp5 = false;
    socklen_t           len;

    bzero(&cliaddr, sizeof(cliaddr));
    len = sizeof(cliaddr);
    tmp.fd = accept(sockfd, (struct sockaddr *)&cliaddr, &len);
    if (tmp.fd != -1)
    {
        if (cli.fds.size() >= 8000)  // do we keep this???
        {
            close(cli.fds.back().fd);
            cli.fds.erase(cli.fds.begin());
                                                    // t'as oas fermè mongole !!!
            cli.bin_buffers.erase(cli.bin_buffers.begin());
            cli.last_mod.erase(cli.last_mod.begin());
            cli.CRLFpos.erase(cli.CRLFpos.begin());
        }
        tmp.events = POLLIN | POLLOUT;
        cli.fds.push_back(tmp);
        cli.bin_buffers.push_back(tmp3);
        gettimeofday(&tmp2, NULL);
        cli.last_mod.push_back(tmp2);
        cli.CRLFpos.push_back(tmp4);
        cli.readReady.push_back(tmp5); // add support for above, 8000 thing???
                                       // I mean a line to erase this btw (int the 8k)
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
        if (get_timediff(*it) > 150)
        {
            handle_connectionerror( 408, cli, inst, i);
        }
        i ++;
    }
    return ;
}

void    server_loop( std::vector<ServInstance> &instances, std::vector<t_server> serv )
{
    int ready_flag = 0;

    while (SIG_HANDLER_GLOBAL == 0) /// jpense faut poll avant accept... jsp si on change ou pas
    {
        for (std::vector<ServInstance>::iterator it = instances.begin(); it != instances.end(); it ++)
        {
            check_IncomingConnections(it->sockfd, it->cli);
            if (it->cli.fds.size() > 0)
            {
                check_timeout(it->cli, *it);
                ready_flag = poll(it->cli.fds.data(), it->cli.fds.size(), 100);//100?
                if (ready_flag < 0)
                    throw std::runtime_error("Polling failed");
                cycle_readFds(it->cli, *it);
                if (it->prio == true || it->cli.fds.size() == 0)
                    continue ;
                cycle_writeFds(it->cli, *it);
            }
        }
    }
    (void)serv;
    return ;
}
