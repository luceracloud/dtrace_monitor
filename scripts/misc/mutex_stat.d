#!/usr/sbin/dtrace -s
#pragma D option quiet

/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  mutex_stat.d
 *   Records stack trace of any mutex locks
 *   that are greater than x ns in length,
 *   given an input PID.
 *
 *   Note one issue - this script assumes
 *   only one mutex lock occurs at a time
 *   per thread.
 *
 *  USAGE: mutex_stat.d [PID] [MIN] [LENGTH] [FRAMES]
 *
 *  [PID]      PID of process to check
 *  [MIN]      Minimum time before recording
 *  [LENGTH]   Number of seconds over which to sample
 *  [FRAMES]   How many stack frames to keep track of
 *
 *  CREATED:   28 JUNE 2013
 *  UPDATED:   28 JUNE 2013
 */

dtrace:::BEGIN
{
  seconds = $3;
  printf("\nCounting time spent with locked mutex in\n");
  printf("process with PID %d over the course of %d seconds\n\n", $1, $4);
  printf("Only saving for elapses greater than %d ns\n\n", $2);
  printf("Note that this program assumes only one\n");
  printf("mutex is acquried at a time.\n\n");
  printf("===========================================");
}

pid$1::pthread_mutex_lock:return
{
  self->thisTid = tid;
  self->sTime = timestamp;
  @P = count();
}

pid$1::pthread_mutex_unlock:entry
/(self->thisTid==(id_t)tid)&&((timestamp-(self->sTime))>$2)/
{
  @D[tid, ustack($4)] = count();
}

pid$1::pthread_mutex_unlock:entry
/self->thisTid==tid/
{
  self->eTime = timestamp;
  @T[tid] = quantize(self->eTime-self->sTime);
}

profile:::tick-1s
{
  seconds--;
}

profile:::tick-1s
/seconds==0/
{
  exit(0);
}

dtrace:::END
{
  printa("\nTotal number of mutex calls: %@d\n", @P);
}
