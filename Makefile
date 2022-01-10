CC       = g++
CCFLAGS  = -std=c++11 -g -O2 -Wall -Wextra -Wmissing-declarations
IFLAGS   =
LDFLAGS  =
LIBFLAGS = -lpthread
TARGETS  = iostat-cgrp

SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

.PHONY:all
	all : $(TARGETS)

iostat-cgrp:$(OBJS)
	$(CC) -o $@ $^ $(LIBFLAGS)

%.o:%.cpp
	$(CC) -o $@ -c $< $(CCFLAGS) $(IFLAGS)

%.d:%.cpp
	@set -e; rm -f $@; $(CC) -MM $< $(IFLAGS) > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(OBJS:.o=.d)

.PHONY:clean
clean:
	rm -f $(TARGETS) *.o *.d *.d.*
