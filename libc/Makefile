LC_SRCS := $(shell find . -name "*.c" -o -name "*.S")

LC_OBJS := $(patsubst ./%.c, $(OBJ_DIR)/libc/%.o, $(filter %.c, $(LC_SRCS)))
LC_OBJS += $(patsubst ./%.S, $(OBJ_DIR)/libc/%.o, $(filter %.S, $(LC_SRCS)))

LC_CFLAGS := $(CFLAGS) -mcmodel=kernel -mno-red-zone \
             -I. \
             -I$(BASE_DIR)/libc/include

.PHONY: all clean

all: $(LC_OBJS)

$(OBJ_DIR)/libc/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	@$(CC) $(LC_CFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR)/libc/%.o: %.S
	@mkdir -p $(dir $@)
	@echo "[AS] $<"
	@$(CC) $(LC_CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)/libc