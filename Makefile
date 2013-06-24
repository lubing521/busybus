###############################################################################
# globals
###############################################################################
CFLAGS =	-Wall -fPIC -O2 -D_GNU_SOURCE -fvisibility=hidden	\
			-Wno-psabi -Wunused
LDFLAGS =	-Wl,-E
DEBUGFLAGS =
LDSOFLAGS =	-shared -rdynamic
CROSS_COMPILE =
CROSSCC =	$(CROSS_COMPILE)$(CC)

###############################################################################
# libbbus.so
###############################################################################
LIBBBUS_OBJS =		error.o						\
			memory.o					\
			protocol.o					\
			server.o					\
			socket.o					\
			object.o
LIBBBUS_TARGET =	./libbbus.so
LIBBBUS_SONAME =	libbbus.so

libbbus.so:		$(LIBBBUS_OBJS)
	$(CROSSCC) -o $(LIBBBUS_TARGET) $(LIBBBUS_OBJS) $(LDFLAGS)	\
		$(DEBUGFLAGS) $(LDSOFLAGS) -Wl,-soname,$(LIBBBUS_SONAME)

###############################################################################
# bbusd
###############################################################################
BBUSD_OBJS =	bbusd.o
BBUSD_TARGET =	./bbusd
BBUSD_LIBS =	-lbbus

bbusd:		$(BBUSD_OBJS)
	$(CROSSCC) -o $(BBUSD_TARGET) $(BBUSD_OBJS) $(LDFLAGS)		\
		$(DEBUGFLAGS) $(BBUSD_LIBS) -L./

###############################################################################
# test
###############################################################################
TEST_OBJS =	./test.o
TEST_TARGET =	./bbus_test

test:		$(TEST_OBJS) $(LIBBBUS_OBJS)
	$(CROSSCC) -o $(TEST_TARGET) $(TEST_OBJS) $(LIBBBUS_OBJS)	\
		$(LDFLAGS) $(DEBUGFLAGS)
	$(TEST_TARGET)

###############################################################################
# all
###############################################################################
all:		libbbus.so bbusd

###############################################################################
# clean
###############################################################################
clean:
	rm -f $(BBUSD_OBJS)
	rm -f $(BBUSD_TARGET)
	rm -f $(LIBBBUS_OBJS)
	rm -f $(LIBBBUS_TARGET)
	rm -f $(TEST_OBJS)
	rm -f $(TEST_TARGET)

###############################################################################
# other
###############################################################################

.PRECIOUS:	%.c
.SUFFIXES:
.SUFFIXES:	.o .c
.PHONY:		all

.c.o:
	$(CROSSCC) -c -o $*.o $(CFLAGS) $(DEBUGFLAGS) $*.c
