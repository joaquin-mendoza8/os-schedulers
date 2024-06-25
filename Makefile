# route to the executable
all:

# # build the executable
# schedule: schedule.c
	@gcc -std=gnu99 -o schedule schedule.c

# run the executable
# test: schedule
# @read -p "File name: " file; \
# ./schedule $$file

	@./schedule test3.txt

# delete the executable
clean:
	@rm -f schedule