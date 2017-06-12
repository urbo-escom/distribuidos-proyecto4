self := $(patsubst %/,%,$(dir $(lastword ${MAKEFILE_LIST})))
target ?= .
dstdir ?= .
srcdir ?= ${self}
exe_suf ?= $(and ${SYSTEMROOT},.exe)


lib := libqueue.a

src :=
src += queue.c
obj := ${src:%.c=%.o}

tst :=
tst += test_queue.c
tstexe := ${tst:%.c=${dstdir}/%${exe_suf}}


.PHONY: ${target}/lib
.PHONY: ${target}/test


${target}/lib: ${dstdir}/${lib}
${target}/test: ${tstexe}


${dstdir}/${lib}: ${obj:%=${dstdir}/%}
	${AR} rcs $@ $(or $?, $^)


${obj:%=${dstdir}/%}: ${dstdir}/%.o: ${srcdir}/%.c
	${CC} -c -o $@ $< ${CFLAGS}


${tstexe}: override LDFLAGS += -lpthread
${tstexe}: ${dstdir}/%${exe_suf}: \
		${srcdir}/%.c ${dstdir}/${lib}
	${CC} -o $@ $^ ${CFLAGS} ${LDFLAGS}
