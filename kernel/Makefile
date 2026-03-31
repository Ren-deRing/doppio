K_SRCS := $(shell find . -name "*.c" -o -name "*.S")

K_OBJS := $(patsubst ./%, $(OBJ_DIR)/kernel/%, $(K_SRCS))
K_OBJS := $(K_OBJS:.c=.o)
K_OBJS := $(K_OBJS:.S=.o)

K_CFLAGS := $(CFLAGS) -mcmodel=kernel -mno-red-zone \
             -I $(BASE_DIR)/include \
             -I $(BASE_DIR) \
             -I $(BASE_DIR)/libc/include \
			 -I $(BASE_DIR)/lib \

.PHONY: all clean

all: $(K_OBJS)

$(OBJ_DIR)/kernel/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "[CC] $<"
	@$(CC) $(K_CFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR)/kernel/%.o: %.S
	@mkdir -p $(dir $@)
	@echo "[AS] $<"
	@$(CC) $(K_CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)/kernel