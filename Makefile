root := $(patsubst %/,%,$(dir $(lastword ${MAKEFILE_LIST})))
target ?= .
srcdir ?= ${root}
dstdir ?= $(or $(and ${SYSTEMROOT},win32),linux)
exe_suf ?= $(and ${SYSTEMROOT},.exe)


inc = \
	$(foreach target, $1, \
	$(foreach srcdir, $1, \
		$(eval include ${root}/${target}/Makefile) \
	) \
	)


$(call inc, fnotify)
$(call inc, queue)
$(call inc, socket)


src :=
src += main.c
src += main_send.c
src += main_recv.c
src += main_monitor.c
src += main_fs.c
src += file.c
obj := ${src:%.c=%.o}


.PHONY: ${target}/all
.DEFAULT_GOAL := ${target}/all
${target}/all: ${dstdir}/main${exe_suf}
${dstdir}/main${exe_suf}: ${dstdir}/libsocket.a
${dstdir}/main${exe_suf}: ${dstdir}/libfnotify.a
${dstdir}/main${exe_suf}: ${dstdir}/libqueue.a
${dstdir}/main${exe_suf}: override LDFLAGS += -lpthread
${dstdir}/main${exe_suf}: override LDFLAGS += $(and ${SYSTEMROOT},-lws2_32)
${dstdir}/main${exe_suf}: ${obj:%=${dstdir}/%} | ${dstdir}/
	${CC} -o $@ $^ ${CFLAGS} ${LDFLAGS}


${obj:%=${dstdir}/%}: override CFLAGS += -I${srcdir}/socket
${obj:%=${dstdir}/%}: override CFLAGS += -I${srcdir}/fnotify
${obj:%=${dstdir}/%}: override CFLAGS += -I${srcdir}/queue
${obj:%=${dstdir}/%}: ${dstdir}/%.o: ${srcdir}/%.c | ${dstdir}/
	${CC} -c -o $@ $< ${CFLAGS}


%/:
	$(if $(wildcard $@.),,mkdir $@)
