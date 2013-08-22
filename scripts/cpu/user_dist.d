#!/usr/sbin/dtrace -s
#pragma D option quiet

/*
 *  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
 *
 *  user_dist.d
 *   Reports distribution of CPU time
 *   across all different users.
 *
 *  USAGE: user_dist.d [INTERVAL] [COUNT]
 *  
 *  [INTERVAL]  Length of interval in seconds
 *  [COUNT]     Number of intervals, 0 for unlimited
 *
 *  CREATED: 30 MAY 2013
 *  UPDATED: 30 MAY 2013
 */

dtrace:::BEGIN
{
  printf("\nUsing intervals of length %d seconds.\n", $1);
  $2 ? printf("Running for %d intervals.\n", $2) :
  printf("Unlimited number of intervals. Press Ctrl-C to kill.\n");
  repeat = $2 ? $2 : 0;		/* Number of intervals  */
  interval = $1;			/* In seconds           */
  number_of_cores = 16;

  countdown = interval;
  rate = 0;
}

/* Make note of running programs */
profile-4999
{
  @P[gid, uid] = count();
}

/* Make note of syscalls */
syscall:::return
{
  @Q[gid, uid] = count();
}


/* Increment counters */
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
/countdown == 0/
{
  repeat--;
  countdown = interval;

  normalize(@P, rate*number_of_cores/100);
  rate = 0;

  printf("\n%-6s %-6s %-8s %-8s\n", "GID", "UID", "USAGE", "SYSCALLS");
  printa("%-6d %-6d %-8@d %-8@d\n", @P, @Q);

  trunc(@P);
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
  printf("\nDone\n");
}

