#pragma once

#include <uv.h>
#include <llhttp.h>

struct HttpHeader {
   uv_buf_t field;
   uv_buf_t value;
};

struct HttpRequest {
   uv_buf_t const uri;
   uv_buf_t const body;
   struct HttpHeader const *headers;
   size_t const header_count;

   enum llhttp_method const method;
   struct {
      uint8_t major;
      uint8_t minor;
   } const version;
   uint8_t const upgrade;

   uv_buf_t const __internal_buffer;
};

typedef void (*uvllhttpd_request_handler) (uv_tcp_t *handle, struct HttpRequest const *request);
//struct HttpRequest* uvllhttpd_request_dup (struct HttpRequest const *request);
//void uvllhttpd_request_free (struct HttpRequest *request);

struct HttpServer {
   uv_tcp_t handle;

   uv_loop_t * const loop;
   uvllhttpd_request_handler const on_request;
   char const * const host;
   unsigned short const port;
   unsigned int const backlog;

   size_t request_buffer_increase_unit;
   size_t request_buffer_max_size;

   llhttp_settings_t _settings;
};

int uvllhttpd_server_listen (struct HttpServer *server);

struct HttpResponse {
   void *data;
   uv_tcp_t *handle;
   uint16_t status;
   uv_buf_t headers;
   uv_buf_t body;
};

struct HttpResponse *uvllhttpd_response_init (uv_tcp_t *handle);
void uvllhttpd_response_add_header (struct HttpResponse *response, char const *s, size_t length);
void uvllhttpd_response_append_body (struct HttpResponse *response, char const *s, size_t length);
void uvllhttpd_response_finish (struct HttpResponse *response);

