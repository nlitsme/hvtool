MYPRJ=.


LDFLAGS+=-g
CFLAGS+=-g -Wall -std=c++1z -D_NO_RAPI -DUSE_STD_REGEX

itslib=$(MYPRJ)/itslib
CFLAGS+=-I $(itslib)/include
CFLAGS+=-I /usr/local/include

regutils=$(MYPRJ)/registryutils
CFLAGS+=-I $(regutils)

hvtool: hvtool.o stringutils.o vectorutils.o debug.o regfileparser.o regvalue.o
	$(CXX) -o $@ $^ $(LDFLAGS)

# find a suitable openssl dir
sslv=$(firstword $(wildcard $(addsuffix /include/openssl/opensslv.h,/usr/local /opt/local $(wildcard /usr/local/opt/openssl*) /usr)))
dirname=$(dir $(patsubst %/,%,$1))
openssl=$(call dirname,$(call dirname,$(call dirname,$(sslv))))

CFLAGS+=-I $(openssl)/include
LDFLAGS+=-L$(openssl)/lib -lcrypto

%.o: %.cpp
	$(CXX) -c -o $@ $^ $(CFLAGS)

%.o: $(itslib)/src/%.cpp
	$(CXX) -c -o $@ $^ $(CFLAGS)

%.o: $(regutils)/%.cpp
	$(CXX) -c -o $@ $^ $(CFLAGS)

clean:
	$(RM) hvtool $(wildcard *.o)
