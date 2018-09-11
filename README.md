# CRC-32C (Castagnoli) for C++ #

This is a hardware-accelerated implementation of CRC-32C (Castagnoli) for Visual C++.
Intel's CRC32 instruction is used if available. Otherwise this library uses fast software fallback.

* [Website](https://crc32c.machinezoo.com/)
* [Tutorial for C++](https://crc32c.machinezoo.com/#cpp)
* [Source code (main repository)](https://bitbucket.org/robertvazan/crc32c-hw/src/default/)
* [zlib license](https://opensource.org/licenses/Zlib)

***

```cpp
uint32_t crc = crc32c_append(0, input, 10000);
```

