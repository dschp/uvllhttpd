# uvllhttpd
HTTP server library utilizing [libuv](https://github.com/libuv/libuv) and [llhttp](https://github.com/nodejs/llhttp) in C

Inspired by [httpserver.h](https://github.com/jeremycw/httpserver.h), this is a very simple C library.
By being based on libuv, you can utilize libuv's various asynchronous supports such as thread pool, timer, and so on.
I try not to hide libuv's API as much as possible, leaving nothing under the hood.

