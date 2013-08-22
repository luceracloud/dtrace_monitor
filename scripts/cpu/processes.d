#!/usr/sbin/dtrace -s
#pragma D option quiet

/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  processes.d
 *   Snapshot running processes for n-
 *   intervals of m-second length. Prints
 *   PID, Process Name, [CPU], Resource
 *   Usage [CPU/All].
 *
 *  USAGE: processes.d [CPUs] [INTERVAL] [COUNT]
 *
 *  [CPUs]      Non-zero to consider each CPU separately
 *  [INTERVAL]  Length of interval in seconds
 *  [COUNT]     Number of intervals. 0 for unlimited
 *
 *  CREATED: 29 MAY 2013
 *  UPDATED: 30 MAY 2013
 */

dtrace:::BEGIN
{
  $1 ? printf("\nConsidering different CPUs\n") :
    printf("\nLumping all CPUs together\n");
  printf("Using intervals of length %d seconds.\n", $2);
  $3 ? printf("Running for %d intervals.\n", $3) :
    printf("Unlimited number of intervals. Press Ctrl-C to kill.\n");
  repeat = $3 ? $3 : -1;  /* Number of intervals        */
  interval = $2;          /* In seconds                 */
  CPU = $1;               /* Consider CPU?              */
  CPUs = 16;              /* Total number of CPUs       */

  countdown = interval;
  rate = 0;
}

/* Make note of running programs */
profile-4999
{
  @P[pid, execname] = count();
  @Q[pid, execname, cpu] = count();
}

/* Increment counter */
tick-4999
{
  rate++;
}

/* Decrement counters */
tick-1s 
{
  countdown--;
}

/* Print after each interval */
tick-1s
/countdown == 0 && !CPU/
{
  repeat--;
  countdown = interval;

  normalize(@P, rate/100*CPUs);
  rate = 0;

  printf("\n%-6s %-16s %11s\n", "PID", "PROCESS NAME", "USAGE(ALL)");
  printa("%-6d %-16s %11@d\n", @P);
  
  trunc(@P);
}

/* Print after each interval */
tick-1s
/countdown == 0 && CPU/
{
  repeat--;
  countdown = interval;

  normalize(@Q, rate/100);
  rate = 0;
  
  printf("\n%-6s %-16s %6s %11s\n", "PID", "PROCESS NAME", "CPU", "USAGE(CPU)");
  printa("%-6d %-16s %6d %11@d\n", @Q);
  
  trunc(@Q);
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
  trunc(@Q);
  printf("\nDone\n");
}
