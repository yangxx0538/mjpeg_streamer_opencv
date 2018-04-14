
CC = gcc
TARGET = test
CFLAGS = -Wall -O2
#INCLUDE = -I /mnt/sda4/public/IPNC_RDK_DM812x_DM385_v3.8.0/Source/ti_tools/ipnc_psp_arago/kernel/include/ -I./
test:
	$(CC) $(INCLUDE) -o $@  TTYIO.c test.c

	
clean:
	@rm -rf *.o $(TARGET)
	@echo "CLEANED"
