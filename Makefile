# Note: requires a 64-bit x86-64 system 
CC = gcc
CFLAGS = -Wall -std=gnu99 -m64 -g

# Adjust these for your own tests!
TEST_S = 4
TEST_W = 8
TEST_b = 4
TEST_V = -v

CSIM_FLAGS =  -s $(TEST_S) -W $(TEST_W) -b $(TEST_b) $(TEST_V)

TRACES = $(wildcard traces/*)
TESTS = $(patsubst traces/%, %.test, $(TRACES))
REF_OUTPUT = $(patsubst traces/%, output/%.test.ref, $(TRACES))
OUR_OUTPUT = $(patsubst traces/%, output/%.test.yours, $(TRACES))

.PHONY: runtests

all: csim

runtests: $(TESTS)

$(TESTS): %.test: traces/% csim output/
	@echo $@
	@./csim $(CSIM_FLAGS) -t $< > output/$@.yours
	@./csim-ref $(CSIM_FLAGS) -t $< > output/$@.ref
	@diff -y output/$@.ref output/$@.yours > output/$@.diff && truncate output/$@.diff -s 0 || true
	@sh -c "if [ -s output/$@.diff ] ; then echo FAILED ; \
		echo See output/$@.{ref,yours,diff} ; \
		else echo PASSED ; fi"

output/:
	mkdir -p output

csim: csim.c
	$(CC) $(CFLAGS) -o csim csim.c -lm 

# Clean the src dirctory
clean:
	rm -f csim
	rm -rf output/
