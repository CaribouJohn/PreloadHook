IDIR =include
CXX=g++
CFLAGS=-I$(IDIR) -std=c++11 -fPIC -D_GNU_SOURCE
ODIR=obj
LDIR =lib

#_DEPS = malloc_override.h
#DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = malloc_override.o 
SOOBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

_TOBJ = test.o 
TESTOBJ = $(patsubst %,$(ODIR)/%,$(_TOBJ))

$(ODIR)/%.o: %.cpp #$(DEPS)
	$(CXX) -c -o $@ $< $(CFLAGS)

malloc_override: $(SOOBJ)
	$(CXX) -shared -o $(LDIR)/$@.so $^ -ldl $(CFLAGS)

test: $(TESTOBJ)
	$(CXX) -o $@ $^ $(CFLAGS)

all: test malloc_override 

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(IDIR)/*~ test $(LDIR)/*.so
