# CRC-32C (Castagnoli) for C++ #

This is a hardware-accelerated implementation of CRC-32C (Castagnoli) for Visual C++.
Intel's CRC32 instruction is used if available. Otherwise this library uses fast software fallback.

* Documentation: [Home](https://crc32c.machinezoo.com/), [Tutorial for C++](https://crc32c.machinezoo.com/#cpp)
* Download: see [Tutorial for C++](https://crc32c.machinezoo.com/#cpp)
* Sources: [GitHub](https://github.com/robertvazan/crc32c-hw), [Bitbucket](https://bitbucket.org/robertvazan/crc32c-hw)
* Issues: [GitHub](https://github.com/robertvazan/crc32c-hw/issues), [Bitbucket](https://bitbucket.org/robertvazan/crc32c-hw/issues)
* License: [zlib license](https://opensource.org/licenses/Zlib)

***

```cpp
uint32_t crc = crc32c_append(0, input, 10000);
```

