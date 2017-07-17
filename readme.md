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
> bin\ntsb_asio 5 20 4096 127.0.0.1 8080
[asio] 127.0.0.1:8080 (20 threads, 4096 messages)
1/5: 640.0MiB in 2567ms (249.3 MiB/s)
2/5: 640.0MiB in 2269ms (282.0 MiB/s)
3/5: 640.0MiB in 2596ms (246.5 MiB/s)
4/5: 640.0MiB in 2397ms (267.0 MiB/s)
5/5: 640.0MiB in 2108ms (303.6 MiB/s)
```

```cmd
> bin\ntsb_net 5 20 4096 127.0.0.1 8080
[net] 127.0.0.1:8080 (20 threads, 4096 messages)
1/5: 640.0MiB in 2211ms (289.4 MiB/s)
2/5: 640.0MiB in 2645ms (241.9 MiB/s)
3/5: 640.0MiB in 2430ms (263.3 MiB/s)
4/5: 640.0MiB in 2523ms (253.7 MiB/s)
5/5: 640.0MiB in 2170ms (294.8 MiB/s)
```

WSL @ Windows 10 Pro 64-bit, Intel Core i7-5500U @ 2.4GHz, 8 GiB RAM

```sh
$ bin/ntsb_asio 5 20 4096 127.0.0.1 8080
[asio] 127.0.0.1:8080 (20 threads, 4096 messages)
1/5: 640.0MiB in 6121ms (104.5 MiB/s)
2/5: 640.0MiB in 6093ms (105.0 MiB/s)
3/5: 640.0MiB in 6004ms (106.6 MiB/s)
4/5: 640.0MiB in 6057ms (105.7 MiB/s)
5/5: 640.0MiB in 6888ms (92.9 MiB/s)
```

```sh
$ bin/ntsb_net 5 20 4096 127.0.0.1 8080
[net] 127.0.0.1:8080 (20 threads, 4096 messages)
1/5: 640.0MiB in 6193ms (103.3 MiB/s)
2/5: 640.0MiB in 6923ms (92.4 MiB/s)
3/5: 640.0MiB in 6983ms (91.6 MiB/s)
4/5: 640.0MiB in 6896ms (92.8 MiB/s)
5/5: 640.0MiB in 6941ms (92.2 MiB/s)
```

Ubuntu 16.04.2 LTS 64-bit, ESXi 5.5.0, Intel Xeon E5-2609 @ 2.5GHz, 8 Cores, 4GiB RAM

```sh
$ bin/ntsb_asio 5 20 4096 127.0.0.1 8080
[asio] 127.0.0.1:8080 (20 threads, 4096 messages)
1/5: 640.0MiB in 544ms (1175.7 MiB/s)
2/5: 640.0MiB in 462ms (1383.4 MiB/s)
3/5: 640.0MiB in 470ms (1359.0 MiB/s)
4/5: 640.0MiB in 493ms (1296.5 MiB/s)
5/5: 640.0MiB in 472ms (1354.0 MiB/s)
```

```sh
$ bin/ntsb_net 5 20 4096 127.0.0.1 8080
[net] 127.0.0.1:8080 (20 threads, 4096 messages)
1/5: 640.0MiB in 603ms (1060.7 MiB/s)
2/5: 640.0MiB in 513ms (1245.7 MiB/s)
3/5: 640.0MiB in 467ms (1369.9 MiB/s)
4/5: 640.0MiB in 460ms (1388.9 MiB/s)
5/5: 640.0MiB in 468ms (1366.7 MiB/s)
```

FreeBSD 11.0-RELEASE-p9 64-bit, ESXi 5.5.0, Intel Xeon E5-2609 @ 2.5GHz, 8 Cores, 4GiB RAM

```sh
$ bin/ntsb_asio 5 20 4096 127.0.0.1 8080
[asio] 127.0.0.1:8080 (20 threads, 4096 messages)
1/5: 640.0MiB in 468ms (1366.1 MiB/s)
2/5: 640.0MiB in 454ms (1407.4 MiB/s)
3/5: 640.0MiB in 459ms (1392.7 MiB/s)
4/5: 640.0MiB in 468ms (1365.9 MiB/s)
5/5: 640.0MiB in 470ms (1361.2 MiB/s)
```

```sh
$ bin/ntsb_net 5 20 4096 127.0.0.1 8080
[net] 127.0.0.1:8080 (20 threads, 4096 messages)
1/5: 640.0MiB in 457ms (1398.5 MiB/s)
2/5: 640.0MiB in 447ms (1429.7 MiB/s)
3/5: 640.0MiB in 433ms (1476.9 MiB/s)
4/5: 640.0MiB in 476ms (1341.7 MiB/s)
5/5: 640.0MiB in 459ms (1394.1 MiB/s)
```
