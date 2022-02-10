TARGET = nnc

CC = clang-13

.DELETE_ON_ERROR :

$(TARGET) : $(TARGET).c heap_top.inc bootstrap.inc
#	$(CC) $< -o $@ -Weverything -g -Os -fsanitize=undefined,address,leak
#	$(CC) $< -o $@ -Weverything -g -Os -fsanitize=undefined,memory
	$(CC) $< -o $@ -Weverything -g -Ofast -march=native -DNDEBUG

.PHONY : clean
clean :
	$(RM) $(TARGET)