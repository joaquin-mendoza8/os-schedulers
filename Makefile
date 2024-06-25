# route to the executable
all:

# build the executable
	@gcc -std=gnu99 -o schedule schedule.c

# run the executable
test: schedule

# ask for file name to test with
	@read -p "File name: " file; \
	./schedule $$file


# delete the executable
clean:
	@rm -f schedule