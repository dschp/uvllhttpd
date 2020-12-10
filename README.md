# uvllhttpd
Asynchronous HTTP server library utilizing [libuv](https://github.com/libuv/libuv) and [llhttp](https://github.com/nodejs/llhttp) in C

Inspired by [httpserver.h](https://github.com/jeremycw/httpserver.h), this is a very simple C library.

By being based on libuv, you can utilize libuv's various asynchronous supports such as thread pool, timer, and so on.
I try not to hide libuv's API as much as possible, and try to keep it as simple as possible and leave nothing under the hood.

By utilizing llhttp, high performance HTTP parsing library, thus without reinventing the wheel, 
this library just focuses on providing a simple and thin API.

## Dependency
- libuv (v1.40.0)
- llhttp (v3.0.0)

For unit testing
- [cgreen](https://github.com/cgreen-devs/cgreen) (1.3.0)
