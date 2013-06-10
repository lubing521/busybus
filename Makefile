###############################################################################
# globals
###############################################################################
CFLAGS =	-Wall -fPIC -O2 -D_GNU_SOURCE -fvisibility=hidden -Wno-psabi
LDFLAGS =	-Wl,-E
DEBUGFLAGS =
LDSOFLAGS =	-shared -rdynamic
CROSS_COMPILE =
COMPILER =	$(CROSS_COMPILE)$(CC)

###############################################################################
# libbbus.so
###############################################################################
LIBBBUS_OBJS =		./error.o					\
			./memory.o					\
			./protocol.o					\
			./server.o					\
			./socket.o
LIBBBUS_TARGET =	libbbus.so
LIBBBUS_SONAME =	libbbus.so

$(LIBBBUS_TARGET):	$(LIBBBUS_OBJS)
	$(CC) -o $(LIBBBUS_TARGET) $(LIBBBUS_OBJS) $(LDFLAGS)		\
		$(LDSOFLAGS) -Wl,-soname,$(LIBBBUS_SONAME)

###############################################################################
# bbusd
###############################################################################
BBUSD_OBJS =	./bbusd.o
BBUSD_TARGET =	bbusd
BBUSD_LIBS =	-lbbus

$(BBUSD_TARGET):	$(BBUSD_OBJS)
	$(CC) -o $(BBUSD_TARGET) $(BBUSD_OBJS) $(LDFLAGS)		\
		$(DEBUGFLAGS) $(BBUSD_LIBS) -L./

###############################################################################
# all
###############################################################################
all:		$(LIBBBUS_TARGET) $(BBUSD_TARGET)

###############################################################################
# clean
###############################################################################
clean:
	rm -f $(BBUSD_OBJS)
	rm -f $(BBUSD_TARGET)
	rm -f $(LIBBBUS_OBJS)
	rm -f $(LIBBBUS_TARGET)

###############################################################################
# other
###############################################################################

.PRECIOUS:	%.c
.SUFFIXES:
.SUFFIXES:	.o .c
.PHONY:		all

.c.o:
	$(COMPILER) -c -o $*.o $(CFLAGS) $(DEBUGFLAGS) $*.c
