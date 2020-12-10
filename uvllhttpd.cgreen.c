#include <string.h>

#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "uvllhttpd.h"
#include "uvllhttpd.impl.h"


static struct HttpServer make_default_server (uvllhttpd_request_handler handler)
{
   struct HttpServer server = {
      .on_request = handler,
   };
   server.request_buffer_increase_unit = 0;
   server.request_buffer_max_size = 10240;

   return server;
}


Describe(HttpServer);
BeforeEach(HttpServer) {}
AfterEach(HttpServer) {}


int uv_tcp_init (uv_loop_t* loop, uv_tcp_t* handle)
{
   return (int) mock (loop, handle);
}

int uv_tcp_bind (uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags)
{
   return (int) mock (handle, addr, flags);
}

int uv_listen (uv_stream_t* stream, int backlog, uv_connection_cb cb)
{
   return (int) mock (stream, backlog, cb);
}

int uv_run (uv_loop_t* loop, uv_run_mode mode)
{
   return (int) mock (loop, mode);
}

void uv_close (uv_handle_t* handle, uv_close_cb close_cb)
{
   mock (handle, close_cb);
}

static uv_buf_t write_buffer;
int uv_write (
      uv_write_t* req,
      uv_stream_t* handle,
      const uv_buf_t bufs[],
      unsigned int nbufs,
      uv_write_cb cb)
{
   int r = (int) mock (req, handle, bufs, nbufs, cb);

   size_t sum_length = write_buffer.len;
   for (unsigned int i = 0; i < nbufs; i++) sum_length += bufs[i].len;

   write_buffer.base = realloc (write_buffer.base, sum_length);

   //size_t pos = write_buffer.len;
   char *p = write_buffer.base + write_buffer.len;
   for (unsigned int i = 0; i < nbufs; i++)
   {
      memcpy (p, bufs[i].base, bufs[i].len);
      p += bufs[i].len;
   }

   return r;
}

static uv_loop_t dummy_loop = {0};
static uvllhttpd_client_t test_client;
static void dummy_request_handler (uv_tcp_t *handle, struct HttpRequest const *request) {}


static void mock_handler_get_simplest (uv_tcp_t *handle, struct HttpRequest const *request)
{
   mock (handle, request);

   assert_that (handle, is_equal_to (&test_client));

   assert_that (request, is_not_null);

   assert_that (request->uri.base, is_equal_to_string ("/"));
   assert_that (request->uri.len, is_equal_to (1));

   assert_that (llhttp_method_name(request->method), is_equal_to_string ("GET"));
}

Ensure(HttpServer, get_simplest)
{
   llhttp_settings_t settings = uvllhttpd_get_llhttp_settings ();
   struct HttpServer server = make_default_server (mock_handler_get_simplest);

   test_client = (uvllhttpd_client_t) {0};
   test_client.server = &server;
   llhttp_init (&(test_client.parser), HTTP_REQUEST, &settings);
   test_client.parser.data = &test_client;
   
   const char* string = "GET / HTTP/1.1\r\n\r\n";
   int string_len = strlen(string);

   expect (mock_handler_get_simplest);

   enum llhttp_errno err = llhttp_execute(&(test_client.parser), string, string_len);
   assert_that (err, is_equal_to (HPE_OK));
}

static void mock_handler_get_some_uri_with_1header (uv_tcp_t *handle, struct HttpRequest const *request)
{
   mock (handle, request);

   assert_that (handle, is_equal_to (&test_client));

   assert_that (request, is_not_null);

   assert_that (request->uri.base, is_equal_to_string ("/helloworld"));
   assert_that (request->uri.len, is_equal_to (11));

   assert_that (request->headers, is_not_null);
   assert_that (request->header_count, is_equal_to (1));

   assert_that (request->headers[0].field.base, is_equal_to_string ("Hello"));
   assert_that (request->headers[0].field.len, is_equal_to (5));
   assert_that (request->headers[0].value.base, is_equal_to_string ("World"));
   assert_that (request->headers[0].value.len, is_equal_to (5));

   assert_that (llhttp_method_name(request->method), is_equal_to_string ("GET"));
}

Ensure(HttpServer, get_some_uri_with_1header)
{
   llhttp_settings_t settings = uvllhttpd_get_llhttp_settings ();
   struct HttpServer server = make_default_server (mock_handler_get_some_uri_with_1header);

   test_client = (uvllhttpd_client_t) {0};
   test_client.server = &server;
   llhttp_init (&(test_client.parser), HTTP_REQUEST, &settings);
   test_client.parser.data = &test_client;
   
   const char* string = "GET /helloworld HTTP/1.1\r\nHello: World\r\n\r\n";
   int string_len = strlen(string);

   expect (mock_handler_get_some_uri_with_1header);

   enum llhttp_errno err = llhttp_execute(&(test_client.parser), string, string_len);
   assert_that (err, is_equal_to (HPE_OK));
}

static void mock_handler_get_some_uri_with_3header (uv_tcp_t *handle, struct HttpRequest const *request)
{
   mock (handle, request);

   assert_that (handle, is_equal_to (&test_client));

   assert_that (request, is_not_null);

   assert_that (request->uri.base, is_equal_to_string ("/helloworld"));
   assert_that (request->uri.len, is_equal_to (11));

   assert_that (request->headers, is_not_null);
   assert_that (request->header_count, is_equal_to (3));

   assert_that (request->headers[0].field.base, is_equal_to_string ("Hello"));
   assert_that (request->headers[0].field.len, is_equal_to (5));
   assert_that (request->headers[0].value.base, is_equal_to_string ("World"));
   assert_that (request->headers[0].value.len, is_equal_to (5));

   assert_that (request->headers[1].field.base, is_equal_to_string ("Host"));
   assert_that (request->headers[1].field.len, is_equal_to (4));
   assert_that (request->headers[1].value.base, is_equal_to_string ("localhost:8080"));
   assert_that (request->headers[1].value.len, is_equal_to (14));

   assert_that (request->headers[2].field.base, is_equal_to_string ("User-Agent"));
   assert_that (request->headers[2].field.len, is_equal_to (10));
   assert_that (request->headers[2].value.base, is_equal_to_string ("foobar"));
   assert_that (request->headers[2].value.len, is_equal_to (6));

   assert_that (llhttp_method_name(request->method), is_equal_to_string ("GET"));
}

Ensure(HttpServer, get_some_uri_with_3header)
{
   llhttp_settings_t settings = uvllhttpd_get_llhttp_settings ();
   struct HttpServer server = make_default_server (mock_handler_get_some_uri_with_3header);

   test_client = (uvllhttpd_client_t) {0};
   test_client.server = &server;
   llhttp_init (&(test_client.parser), HTTP_REQUEST, &settings);
   test_client.parser.data = &test_client;
   
   const char* string = "GET /helloworld HTTP/1.1\r\n"
      "Hello: World\r\n"
      "Host: localhost:8080\r\n"
      "User-Agent: foobar\r\n"
      "\r\n\r\n";
   int string_len = strlen(string);

   expect (mock_handler_get_some_uri_with_3header);

   enum llhttp_errno err = llhttp_execute(&(test_client.parser), string, string_len);
   assert_that (err, is_equal_to (HPE_OK));
}

static void mock_handler_post_simplest (uv_tcp_t *handle, struct HttpRequest const *request)
{
   mock (handle, request);

   assert_that (handle, is_equal_to (&test_client));

   assert_that (request, is_not_null);

   assert_that (request->uri.base, is_equal_to_string ("/helloworld"));
   assert_that (llhttp_method_name(request->method), is_equal_to_string ("POST"));
   assert_that (request->body.base, is_equal_to_string ("Hello World"));
}

Ensure(HttpServer, post_simplest)
{
   llhttp_settings_t settings = uvllhttpd_get_llhttp_settings ();
   struct HttpServer server = make_default_server (mock_handler_post_simplest);

   test_client = (uvllhttpd_client_t) {0};
   test_client.server = &server;
   llhttp_init (&(test_client.parser), HTTP_REQUEST, &settings);
   test_client.parser.data = &test_client;
   
   const char* string = "POST /helloworld HTTP/1.1\r\nContent-Length: 11\r\n\r\nHello World\r\n";
   int string_len = strlen(string);

   expect (mock_handler_post_simplest);

   enum llhttp_errno err = llhttp_execute(&(test_client.parser), string, string_len);
   assert_that (err, is_equal_to (HPE_OK));
}

static void mock_handler_request_buffer_excess_limit (uv_tcp_t *handle, struct HttpRequest const *request)
{
   mock (handle, request);
}

Ensure(HttpServer, request_buffer_excess_limit)
{
   llhttp_settings_t settings = uvllhttpd_get_llhttp_settings ();
   struct HttpServer server = make_default_server (mock_handler_request_buffer_excess_limit);
   server.request_buffer_max_size = 10;

   test_client.server = &server;
   llhttp_init (&(test_client.parser), HTTP_REQUEST, &settings);
   test_client.parser.data = &test_client;
   
   const char* string = "GET /helloworld HTTP/1.1\r\nHello: World\r\n\r\n";
   int string_len = strlen(string);

   expect (uv_close);
   never_expect (mock_handler_request_buffer_excess_limit);

   enum llhttp_errno err = llhttp_execute(&(test_client.parser), string, string_len);
   assert_that (err, is_equal_to (HPE_OK));
}

static void mock_handler_successive_requests (uv_tcp_t *handle, struct HttpRequest const *request)
{
   mock (handle, request);

   assert_that (handle, is_equal_to (&test_client));

   assert_that (request, is_not_null);

   assert_that (request->uri.base, is_equal_to_string ("/"));
   assert_that (request->uri.len, is_equal_to (1));

   assert_that (llhttp_method_name(request->method), is_equal_to_string ("GET"));
}

Ensure(HttpServer, successive_requests)
{
   llhttp_settings_t settings = uvllhttpd_get_llhttp_settings ();
   struct HttpServer server = make_default_server (mock_handler_successive_requests);

   test_client.server = &server;
   llhttp_init (&(test_client.parser), HTTP_REQUEST, &settings);
   test_client.parser.data = &test_client;
   
   const char* string = "GET / HTTP/1.1\r\n\r\n" "GET / HTTP/1.1\r\n\r\n";
   int string_len = strlen(string);

   expect (mock_handler_successive_requests);
   expect (mock_handler_successive_requests);

   enum llhttp_errno err = llhttp_execute(&(test_client.parser), string, string_len);
   assert_that (err, is_equal_to (HPE_OK));
}

Ensure(HttpServer, check_server_init_nullity)
{
   int r = uvllhttpd_server_listen (NULL);
   assert_that (r, is_equal_to (UV_EINVAL));
}

Ensure(HttpServer, check_server_without_buffer_settings)
{
   struct HttpServer server = {
      .loop = &dummy_loop,
      .on_request = dummy_request_handler,
   };

   int r = uvllhttpd_server_listen (&server);
   assert_that (r, is_equal_to (UV_EINVAL));
}

Ensure(HttpServer, server_init_with_invalid_host)
{
   struct HttpServer server = {
      .loop = &dummy_loop,
      .on_request = dummy_request_handler,
      .host = "foo", .port = 12345,
      .request_buffer_max_size = 10240,
   };

   int r = uvllhttpd_server_listen (&server);
   assert_that (r, is_equal_to (UV_EINVAL));
}

Ensure(HttpServer, server_init_when_tcp_init_failed)
{
   struct HttpServer server = {
      .loop = &dummy_loop,
      .on_request = dummy_request_handler,
      .host = "127.0.0.1", .port = 12345,
      .request_buffer_max_size = 10240,
   };

   expect (uv_tcp_init,
         when (loop, is_equal_to (&dummy_loop)),
         will_return (UV_EINVAL));

   int r = uvllhttpd_server_listen (&server);
   assert_that (r, is_equal_to (UV_EINVAL));
}

Ensure(HttpServer, server_init_when_tcp_bind_failed)
{
   struct HttpServer server = {
      .loop = &dummy_loop,
      .on_request = dummy_request_handler,
      .host = "127.0.0.1", .port = 12345,
      .request_buffer_max_size = 10240,
   };

   expect (uv_tcp_init,
         when (loop, is_equal_to (&dummy_loop)),
         will_return (0));
   expect (uv_tcp_bind, will_return (UV_EINVAL));

   int r = uvllhttpd_server_listen (&server);
   assert_that (r, is_equal_to (UV_EINVAL));
}

Ensure(HttpServer, server_init_when_tcp_listen_failed)
{
   struct HttpServer server = {
      .loop = &dummy_loop,
      .on_request = dummy_request_handler,
      .host = "127.0.0.1", .port = 12345,
      .request_buffer_max_size = 10240,
   };

   expect (uv_tcp_init,
         when (loop, is_equal_to (&dummy_loop)),
         will_return (0));
   expect (uv_tcp_bind, will_return (0));
   expect (uv_listen,
         when (backlog, is_equal_to (server.backlog)),
         will_return (UV_EINVAL));

   int r = uvllhttpd_server_listen (&server);
   assert_that (r, is_equal_to (UV_EINVAL));
}

Ensure(HttpServer, server_init_when_everything_is_ok)
{
   struct HttpServer server = {
      .loop = &dummy_loop,
      .on_request = dummy_request_handler,
      .host = "127.0.0.1", .port = 12345,
      .request_buffer_max_size = 10240,
   };

   expect (uv_tcp_init,
         when (loop, is_equal_to (&dummy_loop)),
         will_return (0));
   expect (uv_tcp_bind, will_return (0));
   expect (uv_listen,
         when (backlog, is_equal_to (server.backlog)),
         will_return (0));
   never_expect (uv_run);

   int r = uvllhttpd_server_listen (&server);

   assert_that (r, is_equal_to (0));
   assert_that (server.loop, is_equal_to (server.loop));
   assert_that (server.on_request, is_equal_to (dummy_request_handler));
}

Ensure(HttpServer, response_init_with_tcp_handle_null)
{
   struct HttpResponse *response = uvllhttpd_response_init (NULL);
   assert_that (response, is_null);
}

Ensure(HttpServer, response_init_with_tcp_handle_not_null)
{
   uv_tcp_t handle = {0};
   struct HttpResponse *response = uvllhttpd_response_init (&handle);
   assert_that (response, is_not_null);
}

Ensure(HttpServer, response_no_header)
{
   uv_tcp_t handle = {0};
   struct HttpResponse *response = uvllhttpd_response_init (&handle);
   assert_that (response, is_not_null);

   response->status = 200;

   char body[] = "Hello World.";
   uvllhttpd_response_append_body (response, body, sizeof(body)-1);

   expect (uv_write);
   write_buffer.base = NULL;
   write_buffer.len = 0;

   uvllhttpd_response_finish (response);

   assert_that (write_buffer.base, is_equal_to_string (
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 12\r\n"
            "\r\n"
            "Hello World."
            ));
}

Ensure(HttpServer, response_basic_usage)
{
   uv_tcp_t handle = {0};
   struct HttpResponse *response = uvllhttpd_response_init (&handle);
   assert_that (response, is_not_null);

   response->status = 200;
   char contentType[] = "Content-Type: text/plain";
   uvllhttpd_response_add_header (response, contentType, sizeof(contentType)-1);

   char body[] = "Hello World.";
   uvllhttpd_response_append_body (response, body, sizeof(body)-1);

   expect (uv_write);
   write_buffer.base = NULL;
   write_buffer.len = 0;

   uvllhttpd_response_finish (response);

   assert_that (write_buffer.base, is_equal_to_string (
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 12\r\n"
            "\r\n"
            "Hello World."
            ));
}
