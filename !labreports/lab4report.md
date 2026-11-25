# Lab 4 Report - 12/3/26
### CSEN 383
### Zachary Common and Calianna Collins
## Question 1
The verifier guarantees that all code can be run without errors, but it does not guarantee that the code returns correct values. Only impossible values are detected (such as NULL values).
## Question 2
An eBPF program hooks into the kernel, and the guard reviews all requests to read and write. The guard does not look at the security bits of the file, but rather checks the inode and dev the of the requesting file, and blocks if it is incorrect
## Question 3
1. They printed:
- EPERM
- OK 
```
1255039 -rw-r--r-- 2 root root 7 Nov 25 23:20 /srv/open/a2.txt
1255039 -rw-r--r-- 2 root root 7 Nov 25 23:20 /srv/vault/a.txt
```
- OK
2. If the link is already created, the read still goes through.
3. A file that protects all files would have to read every inode entry and compare them with those inside the vault.
## Question 4