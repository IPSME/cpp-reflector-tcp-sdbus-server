#TODO: https://stackoverflow.com/questions/53969396/precompiled-headers-not-used-by-gcc-when-building-with-a-makefile

.PHONY: all help devi-build devi-run devi-run-local help
.DEFAULT_GOAL := all

OUT=reflector-tcp-sdbus
#INC=-Icpp-msgenv-sdbus.git -Iinclude
INC=-Iinclude
LIB=$(shell pkg-config --cflags --libs libsystemd)

all: ## build the project, 
	g++ -std=c++17 -Wall -O0 -g -o ${OUT} main.cpp ${INC} ${LIB} -pthread
#	g++ -std=c++17 -Wall -O0 -g -o ${OUT} main.cpp JSON_MsgDiscovery.cpp cpp-msgenv-sdbus.git/IPSME_MsgEnv.cpp ${INC} ${LIB}

clean: ## clean the project, 
	rm -f ${OUT}

help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'
	@echo "\nYou probably want to ... "
	@echo

