self := $(patsubst %/,%,$(dir $(lastword ${MAKEFILE_LIST})))
target ?= .
dstdir ?= .
srcdir ?= ${self}
exe_suf ?= $(and ${SYSTEMROOT},.exe)


lib := libsocket.a

src :=
src += socket_addr.c
src += socket_creation.c
src += socket_init.c
src += socket_options.c
src += socket_send_recv.c
obj := ${src:%.c=%.o}

tst :=
tst += test_unicast_listener.c
tst += test_unicast_talker.c
tst += test_broadcast_talker.c
tst += test_multicast_listener.c
tstexe := ${tst:%.c=${dstdir}/%${exe_suf}}


.PHONY: ${target}/lib
.PHONY: ${target}/test


${target}/lib: ${dstdir}/${lib}
${target}/test: ${tstexe}


${dstdir}/${lib}: ${obj:%=${dstdir}/%}
	${AR} rcs $@ $(or $?, $^)


${obj:%=${dstdir}/%}: ${dstdir}/%.o: ${srcdir}/%.c
	${CC} -c -o $@ $< ${CFLAGS}


${tstexe}: override LDFLAGS += $(and ${SYSTEMROOT},-lws2_32)
${tstexe}: ${dstdir}/%${exe_suf}: \
		${srcdir}/%.c ${dstdir}/${lib}
	${CC} -o $@ $^ ${CFLAGS} ${LDFLAGS}
