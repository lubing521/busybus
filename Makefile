###############################################################################
# globals
###############################################################################
CFLAGS =	-Wall -Wextra -fPIC -O2 -D_GNU_SOURCE			\
				-fvisibility=hidden -Wno-psabi
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
			object.o					\
			client.o					\
			string.o					\
			crc32.o						\
			hashmap.o
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
# help
###############################################################################
help:
	@echo "Cleaning:"
	@echo "  clean		- delete temporary files created by build"
	@echo
	@echo "Build:"
	@echo "  all		- all executables and libraries"
	@echo "  bbusd		- busybus daemon"
	@echo "  libbbus.so	- busybus library"
	@echo
	@echo "Testing:"
	@echo "  test		- build the test binary and run the test suite"
	@echo

###############################################################################
# other
###############################################################################

.PRECIOUS:	%.c
.SUFFIXES:
.SUFFIXES:	.o .c
.PHONY:		all clean help test

.c.o:
	$(CROSSCC) -c -o $*.o $(CFLAGS) $(DEBUGFLAGS) $*.c
