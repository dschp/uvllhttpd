#pragma once

#include <uv.h>
#include <llhttp.h>

llhttp_settings_t uvllhttpd_get_llhttp_settings (void);

struct string_in_buffer {
   size_t offset;
   size_t length;
};

struct key_value_in_buffer {
   struct string_in_buffer key;
   struct string_in_buffer value;
};

typedef struct {
   uv_tcp_t handle;
   struct HttpServer *server;

   uv_buf_t buffer;

   enum ParserState {
      ParserState_url,
      ParserState_field,
      ParserState_value,
      ParserState_body,
      ParserState_exceed_buffer,
   } cur_status;

   size_t string_begin_pos;
   size_t buffer_cur_pos;

   struct string_in_buffer uri;
   struct key_value_in_buffer *headers;
   size_t header_count;
   size_t header_cur_index;

   llhttp_t parser;
} uvllhttpd_client_t;
