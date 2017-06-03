CC          = g++
CC_FLAGS    = -std=c++11 -pedantic -Wall -Wextra -g -I.
LIBS        = -lpthread

SRCS =  main.m.cpp

scheduler:
	$(CC) $(CC_FLAGS) $(SRCS) $(LIBS)
