# `DASim`

## Building

`DASim` and its components can be built using CMake. Log-levels can be configured via `DAMANAGER_LOGLEVEL` and `DAMEMORY_LOGLEVEL`.

## Repo Strucure
- `damanager`: Source code for the `damanager` memory manager.
- `damemory`: Source code for the `damemory` user library.
- `benchmark`: Benchmarks to measure OS and XPMEM+userfault memory management performance.
- `xpmem-uffd-mm-test`: Small test for XPMEM+userfaultfd memory management.
- `xpmem-uffd-pf-test`: Small test for XPMEM+userfaultfd page fault handling, write-protect, and page fault blocking.
- `experiments`: Various experiments conducted during the development of `DASim`.
