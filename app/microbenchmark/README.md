# Configuring
config_benchmark.h contains the configuration of the benchmark
Make sure to `make clean` after changing this config otherwise the changes will not
propagate.

AMOUNT_OF_PAGES: Length of the buffer
STEP: Which pages to revoke (If step=2, 30,32,34 etc.)
N: Amount of iterations of the benchmark (First revoke 0 pages, then 1, then 2 etc.)
BASE : Page where the buffer starts on



# Building
`make clean`
`make all`
`sudo ./app` || `sudo ./benchmark.sh`