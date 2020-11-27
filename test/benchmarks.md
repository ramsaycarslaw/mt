# Benchmarks

With Ofast build flag
---
Unix time command
```
mt sorts.mt >  
        0.00s user 0.00s system 24% cpu 0.014 total
```

Internal benchmark

| Proc | Best Case | Worst Case |
|-------------------------------|
| Bubble Sort | 9e-6 | 1e-5 |

With O3 build flag
---
Unix time command
```
mt sorts.mt > 
    0.00s user 0.00s system 36% cpu 0.006 total
```

Internal benchmarks
| Proc | Best Case | Worst Case |
|-------------------------------|
| Bubble Sort | 9e-6 | 1.3e-5   |

Competitors
---
## Python
Best case: 1e-4
Worst case: 0.8 


