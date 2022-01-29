TARGET = nnc

CC = clang-13

.DELETE_ON_ERROR :

$(TARGET) : $(TARGET).c
	$(CC) $< -o $@ -Weverything -g -Os -fsanitize=undefined,address,leak
#	$(CC) $< -o $@ -Weverything -g -Os -fsanitize=undefined,memory
#	$(CC) $< -o $@ -Weverything -g -Ofast -DNDEBUG

.PHONY : clean
clean :
	$(RM) $(TARGET)