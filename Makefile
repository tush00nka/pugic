CC = gcc
CFLAGS = -Wall -Wextra -I src
LDFLAGS = -lraylib -lm -lpthread -ldl -lrt -lGL -lpq

SRCS = src/main.c src/sql.c src/tabs.c src/function.c src/font.c src/plotting.c src/plotting_data.c
OBJS = $(SRCS:.c=.o)

main: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f main $(OBJS)

run: main
	./main