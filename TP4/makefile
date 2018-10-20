SHELL   = /bin/bash
CC      = g++
RM      = rm -f
CFLAGS  = -Wall --std=c++14 $$(pkg-config opencv --cflags)
LIBS    = $$(pkg-config opencv --libs)

CFILES  := $(wildcard *.cpp)
EXECS   := $(CFILES:%.cpp=%)

%.o : %.cpp
	$(CC) $(CFLAGS) -c $*.cpp

all :: $(EXECS)

$(EXECS) : % : %.o
	$(CC) -o $@ $^ $(LIBS)

clean ::
	$(RM) *.o *~ $(EXECS) tmp*.*
