
DEL = rm -rf
FILES = $(shell ls *.bpf.c | awk '{split($$0,a,".");print a[1]}')

ARCH ?= $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/' | sed 's/ppc64le/powerpc/' | sed 's/mips.*/mips/')
CLANG_BPF_SYS_INCLUDES = $(shell $(CLANG) -v -E - </dev/null 2>&1 \
	| sed -n '/<...> search starts here:/,/End of search list./{ s| \(/.*\)|-idirafter \1|p }')

all: $(FILES)

$(FILES) : % : %.bpf.c
	$(DEL) $@.bpf.o
	clang -g -O2 -target bpf -D__TARGET_ARCH_$(ARCH) -c $(filter %.c,$^) -o $@.bpf.o
	llvm-strip -g $@.bpf.o
	cp $@.bpf.o ../$@.bpf.o

%.bpf.o : %

clean:
	$(DEL) *.bpf.o
