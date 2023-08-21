CC = gcc
HFLAGS = -g -I"shared/src" -Wall -w
CFLAGS = -g -Wall -w
LIBS = -lcommons
HEADERS = $(shell find src -name "*.h")
SOURCES = $(shell find src -name "*.c")
HOBJECTS = $(patsubst %.h, Debug/%.o, $(HEADERS))
COBJECTS = $(patsubst %.c, Debug/%.o, $(SOURCES))

all: shared

shared: create-Debug-dir $(COBJECTS)
	$(CC) $(CFLAGS) -shared -o Debug/libshared.so $(COBJECTS) $(LIBS)
	$(CC) $(HFLAGS) -shared -o Debug/libshared.so $(HOBJECTS) $(LIBS)

create-Debug-dir:
	mkdir -p Debug/src

Debug/%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@
	$(CC) $(HFLAGS) -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c $< -o $@

clean:
	rm -rf Debug
	
sources:
	@echo $(SOURCES) $(HEADERS)	
