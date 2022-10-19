ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
CMAKE3 := $(shell if which cmake3>/dev/null ; then echo cmake3; else echo cmake; fi;)
BUILD_DIR := build.cmake

.PHONY: all clean

all:
	mkdir -p $(BUILD_DIR)

ifneq ("${Workflow_DIR}x", "x")
	cd $(BUILD_DIR) && $(CMAKE3) -D Workflow_DIR=${Workflow_DIR} $(ROOT_DIR)
else
	cd $(BUILD_DIR) && $(CMAKE3) $(ROOT_DIR)
endif

	make -C $(BUILD_DIR) -f Makefile

clean:
	rm -rf $(BUILD_DIR)
