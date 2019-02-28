# linux_serial_test

traversal test speed ,databits,stopbits,parity

gcc main.c -o ComTest


Valid options:

   -speedtest coma comb     : traversal test speed ,databits,stopbits,parity
   
  -looptest coma comb     : traversal test speed ,databits,stopbits,parity
  
example1: ./ComTest -speedtest /dev/ttyS0 /dev/ttyS1 

example2: ./ComTest -looptest /dev/ttyS0 /dev/ttyS1 
