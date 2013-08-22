#!/usr/sbin/dtrace -s
#pragma D option quiet

/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  load_dist.d
 *   Examine load distribution of running
 *   processes across various CPUs. Prints
 *   CPU and Resource Usage.
 *
 *  USAGE: load_dist.d [INTERVAL] [COUNT]
 *
 *  [INTERVAL]  Length of interval in seconds
 *  [COUNT]                     Number of intervals. 0 for unlimited
 *
 *  CREATED: 29 MAY 2013
 *  UPDATED: 30 MAY 2013
 */

dtrace:::BEGIN
{
  printf("\nRunning for intervals of length %d seconds.\n", $1);
  $2 ? printf("Running for %d intervals.\n", $2) :
  printf("Unlimited number of intervals. Press Ctrl-C to kill.\n");
  repeat = $2;   /* Number of intervals  */
  interval = $1; /* In seconds           */

  TOT = 0;
  countdown = interval;
  samples = 0;
}

/* Make note of running programs */
profile-4999
{
  @P[cpu] = count();
  @Q[cpu] = count();
  TOT++;
}

/* Increment samples */
tick-4999
{
  samples++;
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

  normalize(@P, samples/100);
  normalize(@Q, TOT/100);
  samples = 0;
  PIDs = 0;

  printf("\n%-6s %7s %7s\n", "CPU", "Usage", "% Total");
  setopt("aggsortkey");
  printa("%-6d %7@d %7@d\n", @P, @Q);

  trunc(@P);
  trunc(@Q);
  TOT = 0;
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