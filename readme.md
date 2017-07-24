# Network TS Benchmark
This is a simple benchmark for <https://github.com/chriskohlhoff/networking-ts-impl>.

## Requirements
* CMake 3.8.2 (Modify CMakeLists.txt if you have an older version.)
* Visual Studio 2017 or LLVM/Clang 4.0.0.

## Compilation
On Windows, execute `solution.cmd` and build the `INSTALL` target.

On Linux and FreeBSD, execute `make install`.

## Benchmark Results

Windows 10 Pro 64-bit, Intel Core i7-5500U @ 2.4GHz, 8 GiB RAM

```cmd
> for %s in (a n) do (for %c in (a n) do bin\ntsb %s %c)
a a: 320.0 MiB in 2.098 s, 152.6 MiB/s, min: 0.625 ms, max: 1571.309 ms, avg: 435.851 ms, med: 416.193 ms
a n: 320.0 MiB in 2.218 s, 144.3 MiB/s, min: 0.330 ms, max: 1280.321 ms, avg: 502.230 ms, med: 558.866 ms
n a: 320.0 MiB in 2.017 s, 158.7 MiB/s, min: 0.299 ms, max: 1292.704 ms, avg: 360.643 ms, med: 265.520 ms
n n: 320.0 MiB in 2.091 s, 153.0 MiB/s, min: 0.026 ms, max: 1232.045 ms, avg: 534.025 ms, med: 455.280 ms
```

WSL @ Windows 10 Pro 64-bit, Intel Core i7-5500U @ 2.4GHz, 8 GiB RAM

```sh
$ for s in a n; do for c in a n; do bin/ntsb $s $c; done; done
a a: 320.0 MiB in 6.178 s, 51.8 MiB/s, min: 1.958 ms, max: 548.515 ms, avg: 451.953 ms, med: 480.089 ms
a n: 320.0 MiB in 6.942 s, 46.1 MiB/s, min: 0.736 ms, max: 590.621 ms, avg: 508.912 ms, med: 543.252 ms
n a: 320.0 MiB in 7.007 s, 45.7 MiB/s, min: 0.326 ms, max: 591.358 ms, avg: 509.261 ms, med: 540.681 ms
n n: 320.0 MiB in 6.953 s, 46.0 MiB/s, min: 1.255 ms, max: 587.936 ms, avg: 510.608 ms, med: 550.595 ms
```

Ubuntu 16.04.2 LTS 64-bit, ESXi 5.5.0, Intel Xeon E5-2609 @ 2.5GHz, 8 Cores, 4GiB RAM

```sh
$ for s in a n; do for c in a n; do bin/ntsb $s $c; done; done
a a: 320.0 MiB in 0.473 s, 676.9 MiB/s, min: 0.168 ms, max: 246.465 ms, avg: 130.334 ms, med: 146.056 ms
a n: 320.0 MiB in 0.596 s, 537.0 MiB/s, min: 0.107 ms, max: 290.611 ms, avg: 201.175 ms, med: 216.943 ms
n a: 320.0 MiB in 0.466 s, 686.2 MiB/s, min: 0.216 ms, max: 201.355 ms, avg: 130.084 ms, med: 152.992 ms
n n: 320.0 MiB in 0.468 s, 683.2 MiB/s, min: 0.139 ms, max: 252.804 ms, avg: 145.526 ms, med: 162.944 ms
```

FreeBSD 11.0-RELEASE-p9 64-bit, ESXi 5.5.0, Intel Xeon E5-2609 @ 2.5GHz, 8 Cores, 4GiB RAM

```sh
$ sh -c 'for s in a n; do for c in a n; do bin/ntsb $s $c; done; done'
a a: 320.0 MiB in 0.477 s, 670.8 MiB/s, min: 0.055 ms, max: 108.828 ms, avg: 3.630 ms, med: 3.593 ms
a n: 320.0 MiB in 0.488 s, 655.6 MiB/s, min: 0.065 ms, max: 110.737 ms, avg: 3.682 ms, med: 3.667 ms
n a: 320.0 MiB in 0.539 s, 593.4 MiB/s, min: 0.060 ms, max: 109.634 ms, avg: 3.764 ms, med: 3.606 ms
n n: 320.0 MiB in 0.496 s, 645.8 MiB/s, min: 0.089 ms, max: 110.107 ms, avg: 3.564 ms, med: 3.506 ms
```
