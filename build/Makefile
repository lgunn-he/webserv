# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: lgunn-he<marvin@42lausanne.ch>             +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2023/12/06 15:01:27 by rlevilla          #+#    #+#              #
#    Updated: 2025/02/14 18:25:09 by lgunn-he         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

name = webserv

objects = main.o webserv.o config.o htmlparser.o req.o resp.o setup.o server.o cgiscript.o

CXXFLAGS := -Wall -Werror -Wextra -std=c++98

CXX := c++

all : ../exec/$(name)

../exec/$(name) : $(objects)
	$(CXX) $(CXXFLAGS) $(objects) -o $(name)
	mv $(name) ../exec


#debug: ../exec/debug_target
#../exec/debug_target : CXXFLAGS += -g -O0 -fsanitize=address
#../exec/debug_target : $(objects)
#	$(CXX) $(CXXFLAGS) $(objects) -o debug_target
#	mv debug_target ../exec/debug_target
	

$(objects) : %.o:%.cpp

clean :
	rm $(objects)

fclean : clean
	rm ../exec/$(name)

re: fclean all
