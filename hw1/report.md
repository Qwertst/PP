```bash
clang++-15 -O0 -fno-tree-vectorize -mavx2 hw1.cpp -o hw1
```

```
Naive:     43969413 cycles
SSE:       34735635 cycles (speedup: 1.27x)
AVX:       31090820 cycles (speedup: 1.41x)
```

Скорей всего мы много тратим на ожидание данных из памяти, а не на вычисления. У нас все же не compute-bound задача, поэтому ожидаемого прироста на таких прогонах мы не увидим
