: '
  Copyright (c) 2013 Lucera Financial Infrastructures http://lucerahq.com
  
  netstats.sh

  CREATED:  1 JULY 2013
  UPDATED:  1 JULY 2013
'

echo "Speed (bits/second)"
kstat -c net -n net0 -s ifspeed

echo "Input (packets; bytes)"
kstat -c net -n net0 -s ipackets64
kstat -c net -n net0 -s rbytes64

echo "Output (packets; bytes)"
kstat -c net -n net0 -s opackets64
kstat -c net -n net0 -s obytes64

echo "Errors (input; output)"
kstat -c net -n net0 -s ierrors
kstat -c net -n net0 -s oerrors