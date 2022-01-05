CFLAGS= -I../inc -I./common
LDFLAGS= -lpthread
CC= gcc
OBJSDIR = ./build

.PHONY: all clean

TARGETS = RtuMaster TcpMaster

COMMON_SRCS = $(wildcard ../src/*.c) $(wildcard ./common/*.c)
COMMON_OBJS = $(patsubst %.c,./$(OBJSDIR)/%.o,$(notdir $(COMMON_SRCS)))

SLAVE_SRCS = $(wildcard ./slave/*.c)
SLAVE_OBJS = $(patsubst %.c,./$(OBJSDIR)/%.o,$(notdir $(SLAVE_SRCS)))

all: $(TARGETS)

RtuMaster:$(COMMON_OBJS) ./$(OBJSDIR)/rtu_master.o
	${CC} $^ -g -o $@ ${LDFLAGS}

TcpMaster:$(COMMON_OBJS) ./$(OBJSDIR)/tcp_master.o
	${CC} $^ -g -o $@ ${LDFLAGS}

ModbusSlave:$(COMMON_OBJS) $(SLAVE_OBJS)
	${CC} $^ -g -o $@ ${LDFLAGS}

./$(OBJSDIR)/%.o:../src/%.c
	@if [ ! -d $(OBJSDIR) ]; then \
		mkdir -p $(OBJSDIR); \
	fi
	${CC} -g -c $< -o $@ ${CFLAGS}

./$(OBJSDIR)/%.o:./common/%.c
	${CC} -g -c $< -o $@ ${CFLAGS}

./$(OBJSDIR)/%.o:./rtu_master/%.c
	${CC} -g -c $< -o $@ ${CFLAGS}

./$(OBJSDIR)/%.o:./tcp_master/%.c
	${CC} -g -c $< -o $@ ${CFLAGS}

./$(OBJSDIR)/%.o:./slave/%.c
	${CC} -g -c $< -o $@ ${CFLAGS}

clean:
	$(RM) $(TARGETS)
	$(RM) ./$(OBJSDIR)/*.o
