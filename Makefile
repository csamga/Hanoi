
CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c89 -O0 -ggdb3

src_dir = .
bin_dir = .

src = $(wildcard $(src_dir)/*.c)
hanoi = $(bin_dir)/hanoi

$(hanoi): $(src)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm $(filter-out %.c,$(hanoi))

run:
	./$(hanoi)
