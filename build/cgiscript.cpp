
#include "webserv.hpp"


bool    check_cgibody( std::string *body ) {

    char            sp[7] = {9, 10, 11, 12, 13, ' ', '\0'};
    size_t          pos[6];
    
pos[4] = body->find("<html>");
    pos[5] = body->find("</html>");
    if (pos[4] != 0 || pos[5] != body->length() - 7)
        return (true);
    pos[0] = body->find("<head>");
    pos[1] = body->find("</head>");
    pos[2] = body->find("<body>");
    pos[3] = body->find("</body>");
    if (pos[0] == std::string::npos || pos[1] == std::string::npos 
            || pos[2] == std::string::npos || pos[3] == std::string::npos
            ||!(pos[0] < pos[1] && pos[1] < pos[2] && pos[2] < pos[3]))
        return (true);
    if (((pos[0] - (pos[4] + 6)) != 0 && body->find_first_not_of(sp, pos[4] + 6, pos[0] - (pos[4] + 6)) != std::string::npos)
            || ((pos[2] - (pos[1] + 7)) != 0 && body->find_first_not_of(sp, pos[1] + 7, pos[2] - (pos[1] + 7)) != std::string::npos)
            || ((pos[5] - (pos[3] + 7)) != 0 && body->find_first_not_of(sp, pos[3] + 7, pos[5] - (pos[3] + 7)) != std::string::npos))
        return (true);
    return (false);
}

int exec_cgiscript( std::string exname, int *arr, const Req &r ) {

    int pid;
    char    *pyloc =  (char *)PYTHON_PATH;
    char    *namexec = (char *)exname.c_str();
    char    *pyt[3] = {pyloc, namexec, NULL};
    std::string reqmeth = "REQUEST_METHOD=" + r.getMethod();
    std::string querstr = "QUERY_STRING=" + r.getQuery();
    std::string contlen = "CONTENT_LENGTH=" + std::to_string(r.getBody().size());
    std::string conttyp = "CONTENT_TYPE=" + r.getContentType();
    char    *envs[5] = {(char *)reqmeth.c_str(), (char *)querstr.c_str(), (char *)contlen.c_str(), (char *)conttyp.c_str(), NULL};

    (void)exname;
    pid = fork();
    if (pid == -1)
        return (-2);
    if (pid == 0)
    {
        dup2(arr[3], 1);
        dup2(arr[0], 0);
        dup2(arr[5], 2);
        close(arr[1]);
        close(arr[2]);
        close(arr[4]);
        execve(PYTHON_PATH, (char *const *)pyt, (char *const *)envs);
        throw std::runtime_error("Program execution failed!");
    }
    else
        return (pid);
}
