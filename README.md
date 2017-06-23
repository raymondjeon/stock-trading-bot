# stock-trading-bot
Algorithmic trading using the mean reversion strategy with bollinger bands

To use, first obtain an api key from oanda. In addition, keep note of your account number which can be found in "my account" 
and then click on "manage funds". Your account number should be in the second from the left column.

In trader.c, replace <your api key> with your api key and it is similar for the account number.

To build the executable, simply run the makefile. The executable is called "algorithm".

The structure is,
trader.c makes api calls using curl.
algorithm.c contains an implmentation of the chosen strategy and outputs various information in both the terminal 
and produces an output log to keep track of trades made.
cJSON.c is an installed library
