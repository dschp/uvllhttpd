#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "uvllhttpd.h"
#include "uvllhttpd.impl.h"

static void close_cb (uv_handle_t *handle);
static void alloc_buffer_cb (uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void read_cb (uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);

static void connection_cb (uv_stream_t *handle, int status)
{
   if (status < 0)
   {
      fprintf (stderr, "New connection error %s\n", uv_strerror(status));
      return;
   }

   struct HttpServer *server = (struct HttpServer *)handle;

   uvllhttpd_client_t *client = calloc (1, sizeof(uvllhttpd_client_t));
   uv_tcp_init (server->loop, &(client->handle));
   if (uv_accept (
            (uv_stream_t*) &(server->handle),
            (uv_stream_t*) &(client->handle)
            ) == 0)
   {
      client->server = server;
      llhttp_init (&(client->parser), HTTP_REQUEST, &(server->_settings));
      client->parser.data = client;

      uv_read_start ((uv_stream_t*) &(client->handle), alloc_buffer_cb, read_cb);
   }
}

static void close_cb (uv_handle_t *handle)
{
   uvllhttpd_client_t *client = (uvllhttpd_client_t *)handle;

   if (client->headers != NULL) free (client->headers);
	free (client);
}

static void alloc_buffer_cb (uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	buf->base = (char*) malloc(suggested_size);
	buf->len = suggested_size;
}

static void read_cb (uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
{
   uvllhttpd_client_t *client = (uvllhttpd_client_t *)handle;
	if (nread > 0)
   {
		enum llhttp_errno err = llhttp_execute (&(client->parser), buf->base, nread);
		if (err == HPE_OK)
		{
			// parsed successfully
		}
		else
		{
			fprintf (stderr, "Parse error: %s %s\n",
               llhttp_errno_name (err), client->parser.reason);
         uv_close ((uv_handle_t*) client, close_cb);
		}
	}
	else if (nread < 0)
	{
		if (nread != UV_EOF)
			fprintf (stderr, "Read error %s\n", uv_err_name(nread));
		uv_close ((uv_handle_t*) client, close_cb);
	}

	free (buf->base);
}

static bool fill_data_to_buffer (uvllhttpd_client_t *client, const char *at, size_t length)
{
   int remaining = client->buffer.len - client->buffer_cur_pos;
   if ((int)length > remaining)
   {
      size_t required_minimum = length - remaining + 1;
      size_t new_size = client->buffer.len + (
         (required_minimum > client->server->request_buffer_increase_unit) ?
         required_minimum :
         client->server->request_buffer_increase_unit);
      if (new_size > client->server->request_buffer_max_size)
      {
         client->cur_status = ParserState_exceed_buffer;
         uv_close ((uv_handle_t*) &(client->handle), close_cb);
         return false;
      }

      client->buffer.base = realloc (client->buffer.base, new_size);
      client->buffer.len = new_size;
   }

   memcpy (client->buffer.base + client->buffer_cur_pos, at, length);
   client->buffer_cur_pos += length;
   return true;
}

static void check_header_complete (uvllhttpd_client_t *client)
{
   if (client->cur_status == ParserState_value)
   {
      client->buffer.base[client->buffer_cur_pos++] = '\0';

      client->headers[client->header_cur_index].value.offset = client->string_begin_pos;
      client->headers[client->header_cur_index].value.length =
         client->buffer_cur_pos - client->string_begin_pos - 1;

      client->string_begin_pos = client->buffer_cur_pos;
      client->header_cur_index++;
   }
}

static int uvllhttpd_on_url(llhttp_t* parser, const char *at, size_t length)
{
   uvllhttpd_client_t *client = (uvllhttpd_client_t *)parser->data;
   if (fill_data_to_buffer (client, at, length))
   {
      client->buffer.base[client->buffer_cur_pos++] = '\0';

      client->uri.offset = client->string_begin_pos;
      client->uri.length = length;

      client->string_begin_pos = client->buffer_cur_pos;
      client->cur_status = ParserState_url;
   }
   return 0;
}

static int uvllhttpd_on_header_field(llhttp_t* parser, const char *at, size_t length)
{
   uvllhttpd_client_t *client = (uvllhttpd_client_t *)parser->data;
   if (client->cur_status == ParserState_exceed_buffer) return 0;

   check_header_complete (client);
   if (fill_data_to_buffer (client, at, length))
   {
      client->cur_status = ParserState_field;
   }
   return 0;
}

static int uvllhttpd_on_header_value(llhttp_t* parser, const char *at, size_t length)
{
   uvllhttpd_client_t *client = (uvllhttpd_client_t *)parser->data;
   if (client->cur_status == ParserState_exceed_buffer) return 0;

   if (client->cur_status == ParserState_field)
   {
      client->buffer.base[client->buffer_cur_pos++] = '\0';

      if (client->header_cur_index == client->header_count)
      {
         client->header_count += 10;
         client->headers = (struct key_value_in_buffer*) realloc (
               (void *)client->headers,
               sizeof(struct key_value_in_buffer) * client->header_count);
      }

      client->headers[client->header_cur_index].key.offset = client->string_begin_pos;
      client->headers[client->header_cur_index].key.length =
         client->buffer_cur_pos - client->string_begin_pos - 1;

      client->string_begin_pos = client->buffer_cur_pos;
   }

   if (fill_data_to_buffer (client, at, length))
   {
      client->cur_status = ParserState_value;
   }
   return 0;
}

static int uvllhttpd_on_headers_complete(llhttp_t* parser)
{
   uvllhttpd_client_t *client = (uvllhttpd_client_t *)parser->data;
   if (client->cur_status == ParserState_exceed_buffer) return 0;

   check_header_complete (client);
   return 0;
}

static int uvllhttpd_on_body(llhttp_t* parser, const char *at, size_t length)
{
   uvllhttpd_client_t *client = (uvllhttpd_client_t *)parser->data;
   if (client->cur_status == ParserState_exceed_buffer) return 0;

   if (fill_data_to_buffer (client, at, length))
   {
      client->cur_status = ParserState_body;
   }
   return 0;
}

static int uvllhttpd_on_message_complete(llhttp_t* parser)
{
   uvllhttpd_client_t *client = (uvllhttpd_client_t *)parser->data;
   if (client->cur_status == ParserState_exceed_buffer) return 0;

   size_t header_count = client->header_cur_index;

   struct HttpHeader headers[header_count > 0 ? header_count : 1];
   if (header_count > 0)
   {
      for (size_t i = 0; i < header_count; i++)
      {
         headers[i] = (struct HttpHeader) {
            .field = (uv_buf_t) {
               .base = client->buffer.base + client->headers[i].key.offset,
               .len = client->headers[i].key.length,
            },
            .value = (uv_buf_t) {
               .base = client->buffer.base + client->headers[i].value.offset,
               .len = client->headers[i].value.length,
            },
         };
      }
      free (client->headers);
      client->headers = NULL;
      client->header_cur_index = 0;
      client->header_count = 0;
   }

   uv_buf_t body;
   if (client->cur_status == ParserState_body)
   {
      client->buffer.base[client->buffer_cur_pos] = '\0';

      body.base = client->buffer.base + client->string_begin_pos;
      body.len = client->buffer_cur_pos - client->string_begin_pos;
   }
   else
   {
      body.base = NULL;
      body.len = 0;
   }

   struct HttpRequest request = {
      .__internal_buffer = client->buffer,
      .uri = {
         .base = client->buffer.base + client->uri.offset,
         .len  = client->uri.length,
      },
      .body = body,
      .header_count = header_count,
      .headers = header_count > 0 ? headers : NULL,
      .method = client->parser.method,
      .upgrade = client->parser.upgrade,
      .version = {
         .major = client->parser.http_major,
         .minor = client->parser.http_minor,
      },
   };


   client->cur_status = ParserState_url;
   client->string_begin_pos = 0;
   client->buffer_cur_pos = 0;

   client->uri.offset = 0;
   client->uri.length = 0;
   client->buffer.base = NULL;
   client->buffer.len = 0;

   client->server->on_request (&(client->handle), &request);

   free (request.__internal_buffer.base);

   return 0;
}

llhttp_settings_t uvllhttpd_get_llhttp_settings (void)
{
   llhttp_settings_t settings = (llhttp_settings_t) {0};
   llhttp_settings_init(&settings);

   settings.on_url              = uvllhttpd_on_url;
   settings.on_header_field     = uvllhttpd_on_header_field;
   settings.on_header_value     = uvllhttpd_on_header_value;
   settings.on_headers_complete = uvllhttpd_on_headers_complete;
   settings.on_body             = uvllhttpd_on_body;
   settings.on_message_complete = uvllhttpd_on_message_complete;

   return settings;
}

int uvllhttpd_server_listen (struct HttpServer *server)
{
   if (server == NULL) return UV_EINVAL;
   if (server->loop == NULL || server->on_request == NULL) return UV_EINVAL;
   if (server->request_buffer_max_size == 0) return UV_EINVAL;

   int r;

   struct sockaddr_in addr;
   r = uv_ip4_addr (server->host, server->port, &addr);
   if (r != 0) return r;

   r = uv_tcp_init (server->loop, &(server->handle));
   if (r != 0) return r;

   r = uv_tcp_bind (&(server->handle), (struct sockaddr *) &addr, 0);
   if (r != 0) return r;

   r = uv_listen ((uv_stream_t *) &(server->handle), server->backlog, connection_cb);
   if (r != 0) return r;

	server->_settings = uvllhttpd_get_llhttp_settings ();

   return r;
}

#if 0
void uvllhttpd_request_free (struct HttpRequest *request)
{
   if (request == NULL) return;

   if (request->headers != NULL) free (request->headers);
   if (request->_buffer.base != NULL) free (request->_buffer.base);

   free (request);
}
#endif

struct HttpResponse *uvllhttpd_response_init (uv_tcp_t *handle)
{
   if (handle == NULL) return NULL;

   struct HttpResponse *response = calloc (1, sizeof(struct HttpResponse));
   response->handle = handle;

   return response;
}

void uvllhttpd_response_add_header (struct HttpResponse *response, char const *s, size_t length)
{
   if (response == NULL) return;

   response->headers.base = realloc (response->headers.base, response->headers.len + length + 2);
   memcpy (response->headers.base + response->headers.len, s, length);
   response->headers.len += length;

   response->headers.base[response->headers.len++] = '\r';
   response->headers.base[response->headers.len++] = '\n';
}

void uvllhttpd_response_append_body (struct HttpResponse *response, char const *s, size_t length)
{
   if (response == NULL) return;

   response->body.base = realloc (response->body.base, response->body.len + length);
   memcpy (response->body.base + response->body.len, s, length);
   response->body.len += length;
}

static void write_cb (uv_write_t* req, int status)
{
   struct HttpResponse *response = (struct HttpResponse *)req->data;

   if (response->headers.base != NULL) free (response->headers.base);
   if (response->body.base != NULL) free (response->body.base);

   free (response);
   free (req);
}

void uvllhttpd_response_finish (struct HttpResponse *response)
{
   if (response == NULL) return;

   size_t const buffer_size = 64;
   uv_write_t *req = malloc (sizeof(uv_write_t) + buffer_size*2);
   char * const buffer_base = (char *)req + sizeof(uv_write_t);

   uv_buf_t bufs[4];
   bufs[0].base = buffer_base;
   bufs[0].len = snprintf (bufs[0].base, buffer_size, "HTTP/1.1 %d OK\r\n", response->status);
   bufs[1] = response->headers;
   bufs[2].base = buffer_base + buffer_size;
   bufs[2].len = snprintf (bufs[2].base, buffer_size, "Content-Length: %zd\r\n\r\n", response->body.len);
   bufs[3] = response->body;

   req->data = response;
   uv_write (req, (uv_stream_t *)response->handle, bufs, 4, write_cb);
}
