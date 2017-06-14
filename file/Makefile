self := $(patsubst %/,%,$(dir $(lastword ${MAKEFILE_LIST})))
target ?= .
dstdir ?= .
srcdir ?= ${self}
exe_suf ?= $(and ${SYSTEMROOT},.exe)


lib := libfile.a

src :=
src += file.c
obj := ${src:%.c=${dstdir}/%.o}


.PHONY: ${target}/lib
.PHONY: ${target}/test


${target}/lib: ${dstdir}/${lib}


${dstdir}/${lib}: ${obj}
	${AR} rcs $@ $(or $?, $^)


.INTERMEDIATE: ${obj}
${obj}: ${dstdir}/%.o: ${srcdir}/%.c
	${CC} -c -o $@ $< ${CFLAGS}

