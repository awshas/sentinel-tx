# Printf Cost

When creating a stub main.c with a `printf("Hello World!");` the memory table read:

```
FLASH:  4308 B   512 KB   0.82%
RAM:    1480 B    96 KB   1.51%
CCMRAM:    0 B    32 KB   0.00%
```
Without the `printf()` function the memory table read:

```
FLASH:   376 B       512 KB      0.07%
RAM:    1056 B        96 KB      1.07%
CCMRAM:    0 B        32 KB      0.00%
```
One `printf()` costs **3932 B** flash and **424 B** RAM.