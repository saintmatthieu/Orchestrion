/*
 * Platform wake-up latency test — Orchestrion-free. Pins one thread per CPU,
 * each sleeping 5 ms in a loop, and reports every wake-up more than 30 ms
 * late with its CPU and CLOCK_MONOTONIC time (the same axis as
 * ORCHESTRION_PERF_LOG). Audio dropouts that also appear here are not the
 * app's doing: a timer that does not fire for 200 ms is a firmware (SMI),
 * idle-state or power-management stall of that core.
 *
 *     gcc -O2 -pthread -o /tmp/wakeup buildscripts/perf_wakeup_test.c
 *     /tmp/wakeup 90        # seconds; prints overshoots, then the worst per CPU
 *
 * Seen on a ThinkPad P16 Gen 1 (i7-12800HX): ~200 ms stalls of sibling
 * P-core pairs every 10.75 s; E-cores unaffected.
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
static double now_s(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec + t.tv_nsec*1e-9; }
static double g_end; static FILE* g_out; static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static void* run(void* arg){
  int cpu = (int)(long)arg; cpu_set_t set; CPU_ZERO(&set); CPU_SET(cpu,&set); pthread_setaffinity_np(pthread_self(), sizeof set, &set);
  struct timespec req = {0, 5000000};
  double worst = 0;
  while (now_s() < g_end) {
    double t0 = now_s(); nanosleep(&req, NULL); double dt = now_s() - t0; double over = dt - 0.005;
    if (over > worst) worst = over;
    if (over > 0.030) { pthread_mutex_lock(&g_mu); fprintf(g_out, "%.3f cpu%d overshoot %.1fms\n", t0, cpu, over*1000); pthread_mutex_unlock(&g_mu); }
  }
  pthread_mutex_lock(&g_mu); fprintf(g_out, "done cpu%d worst %.1fms\n", cpu, worst*1000); pthread_mutex_unlock(&g_mu);
  return NULL;
}
int main(int argc, char** argv){
  int secs = argc > 1 ? atoi(argv[1]) : 90; int n = sysconf(_SC_NPROCESSORS_ONLN);
  g_out = stdout; setvbuf(stdout, NULL, _IOLBF, 0); g_end = now_s() + secs;
  fprintf(g_out, "start %.3f cpus %d\n", now_s(), n);
  pthread_t th[256]; for (long i=0;i<n;i++) pthread_create(&th[i], NULL, run, (void*)i);
  for (int i=0;i<n;i++) pthread_join(th[i], NULL);
  return 0;
}
