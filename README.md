# Linux-Daemon
## Description
The daemon moves files each 30 seconds
* from directory1 to directory2: if file's lifetime is more than threshold (2 minutes)
* from directory2 to directory1: if file's lifetime is less than threshold (2 minutes)
## Start program
    ./daemon start
## Stop program
    ./daemon stop
## Test program
Run bash script test.sh
## Config file
    <absolute path to directory1> <absolute path to directory2> <time interval in seconds> 
