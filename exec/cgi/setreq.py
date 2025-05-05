#!/opt/homebrew/bin/python3
import sys


with open("/Users/lgunn-he/Desktop/Stuff/Cursus/projects/webserv/exec/cgi/index.html", "r") as f:
    s = ""
    txt = f.read()
    li = txt.split()
    integer = 0
    for n, i in enumerate(li):
        if i.isnumeric():
            integer = int(i) + 1
            li[n] = str(integer)
            break
    s = ' '.join(li).replace("> ", ">\n")
    f.close()
    with open("/Users/lgunn-he/Desktop/Stuff/Cursus/projects/webserv/exec/cgi/index.html", "w") as fw:
        fw.write(s)
        print(s)
        fw.close()


