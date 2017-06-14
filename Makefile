root := $(patsubst %/,%,$(dir $(lastword ${MAKEFILE_LIST})))
target ?= .
srcdir ?= ${root}
dstdir ?= .
exe_suf ?= $(and ${SYSTEMROOT},.exe)
program ?= kazaa-fs${exe_suf}


.SUFFIXES:
.DELETE_ON_ERROR:


target := file
srcdir := ${root}/${target}
include ${root}/${target}/Makefile

target := fnotify
srcdir := ${root}/${target}
include ${root}/${target}/Makefile

target := queue
srcdir := ${root}/${target}
include ${root}/${target}/Makefile

target := socket
srcdir := ${root}/${target}
include ${root}/${target}/Makefile


target := .
srcdir := ${root}
dstdir := .


src :=
src += main.c
src += main_fd_list.c
src += main_filesystem.c
src += main_monitor.c
src += main_recv.c
src += main_send.c
obj := ${src:%.c=${dstdir}/%.o}


.PHONY: ${target}/all
.PHONY: ${target}/clean
.PHONY: ${target}/run
.PHONY: ${target}/run-clean

.DEFAULT_GOAL := ${target}/all
${target}/all: ${dstdir}/${program}
${target}/clean: dstdir := ${dstdir}
${target}/clean: program := ${program}
${target}/clean::
	rm -rf ${dstdir}/${program}
	rm -rf ${dstdir}/*.o
	rm -rf ${dstdir}/*.a
	rm -rf ${dstdir}/*.exe

${dstdir}/${program}: ${dstdir}/libfile.a
${dstdir}/${program}: ${dstdir}/libfnotify.a
${dstdir}/${program}: ${dstdir}/libqueue.a
${dstdir}/${program}: ${dstdir}/libsocket.a
${dstdir}/${program}: override LDFLAGS += -lpthread
${dstdir}/${program}: override LDFLAGS += $(and ${SYSTEMROOT},-lws2_32)
${dstdir}/${program}: ${obj} | ${dstdir}/
	${CC} -o $@ $^ ${CFLAGS} ${LDFLAGS}

${target}/run: run := $(abspath ${dstdir}/${program})
${target}/run:
	${run}

${target}/run-clean: run := $(abspath ${dstdir}/${program})
${target}/run-clean:
	rm -rf ./kazaa ./kazaa.bak ./kazaa.trash && ${run}


.INTERMEDIATE: ${obj}
${obj}: override CFLAGS += -I${srcdir}/file
${obj}: override CFLAGS += -I${srcdir}/fnotify
${obj}: override CFLAGS += -I${srcdir}/queue
${obj}: override CFLAGS += -I${srcdir}/socket
${obj}: ${dstdir}/%.o: ${srcdir}/%.c ${srcdir}/shfs.h | ${dstdir}/
	${CC} -c -o $@ $< ${CFLAGS}


%/:
	$(if $(wildcard $@.),,mkdir $@)
