LIB_MAKEFILES := $(shell find . -mindepth 2 -maxdepth 2 -name "Makefile")
LIB_DIRS      := $(patsubst %/Makefile, %, $(LIB_MAKEFILES))

.PHONY: all clean $(LIB_DIRS)

all: $(LIB_DIRS)

$(LIB_DIRS):
	@$(MAKE) -C $@

clean:
	@for dir in $(LIB_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done