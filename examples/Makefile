CFLAGS = -I../inc -I../util -I./common
LDFLAGS = -lpthread
CC = gcc -std=gnu99
OBJSDIR = ./build

.PHONY: all clean

TARGETS = ./rtu_master/RtuMaster ./tcp_master/TcpMaster ./slave/ModbusSlave \
		  ./rtu_p2p/p2p_master ./rtu_p2p/p2p_slave \
		  ./rtu_broadcast/broadcast_master ./rtu_broadcast/broadcast_slave

COMMON_SRCS = $(wildcard ../src/*.c) $(wildcard ../util/*.c) $(wildcard ./common/*.c)
COMMON_OBJS = $(patsubst %.c,./$(OBJSDIR)/%.o,$(notdir $(COMMON_SRCS)))

SLAVE_SRCS = $(wildcard ./slave/*.c)
SLAVE_OBJS = $(patsubst %.c,./$(OBJSDIR)/%.o,$(notdir $(SLAVE_SRCS)))

all: $(TARGETS)

./rtu_master/RtuMaster : $(COMMON_OBJS) ./$(OBJSDIR)/rtu_master.o
	${CC} $^ -g -o $@ ${LDFLAGS}

./tcp_master/TcpMaster : $(COMMON_OBJS) ./$(OBJSDIR)/tcp_master.o
	${CC} $^ -g -o $@ ${LDFLAGS}

./slave/ModbusSlave : $(COMMON_OBJS) $(SLAVE_OBJS)
	${CC} $^ -g -o $@ ${LDFLAGS}

./rtu_p2p/p2p_master : $(COMMON_OBJS) ./$(OBJSDIR)/p2p_master.o
	${CC} $^ -g -o $@ ${LDFLAGS}

./rtu_p2p/p2p_slave : $(COMMON_OBJS) ./$(OBJSDIR)/p2p_slave.o
	${CC} $^ -g -o $@ ${LDFLAGS}

./rtu_broadcast/broadcast_master : $(COMMON_OBJS) ./$(OBJSDIR)/broadcast_master.o
	${CC} $^ -g -o $@ ${LDFLAGS}

./rtu_broadcast/broadcast_slave : $(COMMON_OBJS) ./$(OBJSDIR)/broadcast_slave.o
	${CC} $^ -g -o $@ ${LDFLAGS}

./$(OBJSDIR)/%.o : ../src/%.c
	@if [ ! -d $(OBJSDIR) ]; then \
		mkdir -p $(OBJSDIR); \
	fi
	${CC} -g -c $< -o $@ ${CFLAGS}

./$(OBJSDIR)/%.o : ../util/%.c
	${CC} -g -c $< -o $@ ${CFLAGS}

./$(OBJSDIR)/%.o : ./common/%.c
	${CC} -g -c $< -o $@ ${CFLAGS}

./$(OBJSDIR)/%.o : ./rtu_master/%.c
	${CC} -g -c $< -o $@ ${CFLAGS}

./$(OBJSDIR)/%.o : ./tcp_master/%.c
	${CC} -g -c $< -o $@ ${CFLAGS}

./$(OBJSDIR)/%.o : ./slave/%.c
	${CC} -g -c $< -o $@ ${CFLAGS}

./$(OBJSDIR)/%.o : ./rtu_p2p/%.c
	${CC} -g -c $< -o $@ ${CFLAGS}

./$(OBJSDIR)/%.o : ./rtu_broadcast/%.c
	${CC} -g -c $< -o $@ ${CFLAGS}

clean:
	$(RM) $(TARGETS)
	$(RM) ./$(OBJSDIR)/*.o
