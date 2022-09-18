/*
 * Copyright (c) 2022, Ali Mashtizadeh
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#include <sys/param.h>
#include <sys/cpuset.h>

#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <x86intrin.h>

pthread_t pri, sec;
pthread_barrier_t barrier;

void
setcpu(int cpuid)
{
    int status;
    cpuset_t aff;
    int maskSize = sizeof(cpuset_t) * 1;

    status = cpuset_getaffinity(CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, maskSize, &aff);
    if (status < 0) {
	perror("cpuset_getaffinity");
	exit(1);
    }

    // Pin to one CPU
    CPU_ZERO(&aff);
    CPU_SET(cpuid, &aff);

    status = cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, maskSize, &aff);
    if (status < 0) {
	perror("cpuset_setaffinity");
	exit(1);
    }

    // Run the scheduler (not sure we need this)
    pthread_yield(); pthread_yield();

    uint32_t cpu;
    __rdtscp(&cpu);
    if (cpu != cpuid) {
	fprintf(stderr, "cpuid mismatch!\n");
    }
}

volatile uint64_t recv;

void *
pri_thread(void *arg)
{
    recv = 0;
    setcpu(0);
    pthread_barrier_wait(&barrier);

    for (int i = 0; i < 100; i++) {
	uint64_t start = __builtin_readcyclecounter();
	pthread_kill(sec, SIGIO);
	uint64_t stop = __builtin_readcyclecounter();

	while (recv == 0) {}
	printf("Sent %lu, Recv: %lu\n", stop - start, recv - start);
	recv = 0;
    }

    pthread_kill(sec, SIGKILL);

    return NULL;
}

void
sighandler(int signo)
{
    recv = __builtin_readcyclecounter();
}

void *
sec_thread(void *arg)
{
    signal(SIGIO, sighandler);

    setcpu(1);
    pthread_barrier_wait(&barrier);

    while (1) {
    }

    return NULL;
}

int
main(int argc, const char *argv[])
{
    if (pthread_barrier_init(&barrier, NULL, 2) < 0) {
	perror("pthread_barrier");
	exit(1);
    }

    if (pthread_create(&pri, NULL, &pri_thread, NULL) < 0) {
	perror("pthread_create");
	exit(1);
    }
    if (pthread_create(&sec, NULL, &sec_thread, NULL) < 0) {
	perror("pthread_create");
	exit(1);
    }

    pthread_join(pri, NULL);
    pthread_join(sec, NULL);

    return 0;
}

