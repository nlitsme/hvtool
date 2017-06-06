MYPRJ=.


LDFLAGS=-g -m32
CFLAGS=-g -m32 -Wall -std=c++1z -D_NO_RAPI -DUSE_STD_REGEX

itslib=$(MYPRJ)/itslib
CFLAGS+=-I $(itslib)/include

regutils=$(MYPRJ)/registryutils
CFLAGS+=-I $(regutils)

hvtool: hvtool.o stringutils.o vectorutils.o debug.o regfileparser.o regvalue.o
	$(CXX) -o $@ $^ $(LDFLAGS)

openssl=/usr/local/opt/openssl
CFLAGS+=-I $(openssl)/include
LDFLAGS+=-lcrypto

%.o: %.cpp
	$(CXX) -c -o $@ $^ $(CFLAGS)

%.o: $(itslib)/src/%.cpp
	$(CXX) -c -o $@ $^ $(CFLAGS)

%.o: $(regutils)/%.cpp
	$(CXX) -c -o $@ $^ $(CFLAGS)

clean:
	$(RM) hvtool $(wildcard *.o)
