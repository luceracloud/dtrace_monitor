#!/usr/sbin/dtrace -s
#pragma D option quiet

/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  proc_sys.d
 *   Determine system calls made by a
 *   specific process (identified by
 *   PID) and also display time stats
 *   for each syscall.
 *
 *  USAGE: proc_sys.d [PID] [INTERVAL] [COUNT]
 *
 *  [PID]       Process ID
 *  [INTERVAL]  Length of interval in seconds
 *  [COUNT]     Number of intervals. 0 for unlimited
 *
 *  CREATED: 30 MAY 2013
 *  UPDATED: 30 MAY 2013
 */

dtrace:::BEGIN
{
  PID = $1;
  printf("\nLooking at process with PID %d\n", PID);
  printf("Using intervals of length %d seconds.\n", $2);
  $3 ? printf("Running for %d intervals.\n", $3) :
    printf("Unlimited number of intervals. Press Ctrl-C to kill.\n");
  repeat = $3 ? $3 : -1;  /* Number of intervals */
  interval = $2;          /* In seconds */

  countdown = interval;
}

/* Make note of syscalls */
syscall:::entry
/pid == PID/
{
  @P[probefunc, execname] = count();
  self->start = timestamp;
}

/* Record completion of syscall */
syscall:::return 
/pid == PID/
{
  self->stop = timestamp;
  @Qtot[probefunc, execname] = sum(self->stop - self->start);
  @Qavg[probefunc, execname] = avg(self->stop - self->start);
  @Qmin[probefunc, execname] = min(self->stop - self->start);
  @Qmax[probefunc, execname] = max(self->stop - self->start);
}


/* Decrement counters */
tick-1s 
{
  countdown--;
}

/* Print after each interval */
tick-1s
/countdown == 0/
{
  repeat--;
  countdown = interval;

  rate = 0;

  printf("\n%-14s %6s | %12s %10s %10s %10s\n", "SYSCALL", "COUNT", "TIME: TOTAL", "AVERAGE", "MAX", "MIN");
  printa("%-14s %6@d | %12@d %10@d %10@d %10@d\n", @P, @Qtot, @Qavg, @Qmax, @Qmin);

  trunc(@P);
  trunc(@Qtot);
  trunc(@Qavg);
  trunc(@Qmax);
  trunc(@Qmin);
}

/* Exit program */
tick-1s
/repeat == 0/
{
  exit(0);
}

/* Cleanup */
dtrace:::END
{
  trunc(@P);
  trunc(@Qtot);
  trunc(@Qavg);
  trunc(@Qmax);
  trunc(@Qmin);
  printf("\nDone\n");
}