IDIR =include
CXX=g++
CFLAGS=-I$(IDIR) -fPIC -D_GNU_SOURCE
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

apama_malloc: $(SOOBJ)
	$(CXX) -shared -o $(LDIR)/$@.so $^ -ldl $(CFLAGS)

test: $(TESTOBJ)
	$(CXX) -o $@ $^ $(CFLAGS)

all: test apama_malloc 

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(IDIR)/*~ test $(LDIR)/*.so
