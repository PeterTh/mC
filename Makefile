CXX=g++
RM=rm -f
CPPFLAGS=-g3 -O0 -Wall --std=c++14
LDFLAGS=

SRCS=$(shell ls *.cpp) $(shell ls tests/*.cpp)
OBJS=$(subst .cpp,.o,$(SRCS))

all:  $(OBJS)
	$(CXX) $(LDFLAGS) -o mC $(OBJS) $(LDLIBS)

%.o : %.cpp
	$(CXX) $(CPPFLAGS) $< -c -o $@

clean:
	$(RM) $(OBJS)

print-%  : ; @echo $* = $($*)
