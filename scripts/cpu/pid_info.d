#!/usr/sbin/dtrace -s
#pragma D option quiet

/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  pid_info.d
 *   Snapshot running process for n-
 *   intervals of m-second length. Prints
 *   CPU usage, system calls (no & time)
 *
 *  USAGE: pid_info.d [INTERVAL] [COUNT] [PID]
 *
 *  [INTERVAL]  Length of interval in seconds
 *  [COUNT]     Number of intervals. 0 for unlimited
 *  [PID]       PID of process
 *
 *  CREATED: 16 JULY 2013
 *  UPDATED: 16 JULY 2013
 */

dtrace:::BEGIN
{
  printf("\nUsing intervals of length %d seconds.\n", $1);
  $2 ? printf("Running for %d intervals.\n", $2) :
    printf("Unlimited number of intervals. Press Ctrl-C to kill.\n");
  repeat = $2 ? $2 : -1;  /* Number of intervals        */
  interval = $1;          /* In seconds                 */
  CPUs = 16;              /* Total number of CPUs       */

  countdown = interval;
  rate = 0;
}

/* Keep track of which CPU the process is running on */
profile-4999
/pid==$3/
{
  @P[cpu] = count();
}

/* Make note of system calls */
syscall:::entry
/pid==$3/
{
  self->begun = timestamp;
  @Q[probefunc] = count();
}

/* ibid */
syscall:::return
/pid==$3/
{
  self->ended = timestamp;
  @R[probefunc] = avg(self->ended-self->begun);
}

/* Increment counter */
tick-4999
{
  rate++;
}

/* Decrement counter */
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
  
  normalize(@P, rate/100);
  rate = 0;
  printf("\n=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=");
  printf("\n%-4s %9s\n", "CPU", "%% usage");
  printa("% -3d %9@d\n", @P);
  printf("\n%-20s %-12s %-10s\n", "System Call", "Occurrence", "Average duration (ns)");
  printa("%-20s %-12@d %-10@d\n", @Q, @R);
  
  trunc(@P);
  trunc(@Q);
  trunc(@R);
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
  printf("\nDone\n");
}