# Configuring
config_benchmark.h contains the configuration of the benchmark
Make sure to `make clean` after changing this config otherwise the changes will not
propagate.

This benchmark measures the path of triggering a fault to control being given to the user space registered fault handler.



N: Amount of iterations of the benchmark 


# Building
`make clean`
`make run`