# Lab 4 Report - 12/3/26
### CSEN 383
### Zachary Common and Calianna Collins
## Question 1
The verifier guarantees that all code can be run without errors, but it does not guarantee that the code returns correct values. Only impossible values are detected (such as NULL values).
## Question 2
An eBPF program hooks into the kernel, and the guard reviews all requests to read and write. The guard does not look at the security bits of the file, but rather checks the inode and dev the of the requesting file, and blocks if it is incorrect.