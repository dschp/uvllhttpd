#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "uvllhttpd.h"


void write_cb (uv_write_t* req, int status)
{
   free (req);
}

void on_request1 (uv_tcp_t *handle, struct HttpRequest const *request)
{
   printf ("on_request1\n");

   char response[] = "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain\r\n"
      "Content-Length: 12\r\n"
      "\r\n"
      "Hello World.";
   uv_buf_t buf = {
      .base = response, .len = sizeof(response)-1,
   };

   uv_write_t *req = malloc (sizeof(uv_write_t));
   uv_write (req, (uv_stream_t *)handle, &buf, 1, write_cb);
}

void on_request2 (uv_tcp_t *handle, struct HttpRequest const *request)
{
   printf ("on_request2\n");

   struct HttpResponse *response = uvllhttpd_response_init (handle);
   response->status = 200;
   char contentType[] = "Content-Type: text/plain";
   uvllhttpd_response_add_header (response, contentType, sizeof(contentType)-1);

   char body[] = "Hello World";
   uvllhttpd_response_append_body (response, body, sizeof(body)-1);

   uvllhttpd_response_finish (response);
}

struct mywork {
   uv_work_t work;
   uv_tcp_t *handle;
   char uri[100];
};

void work_cb (uv_work_t* work)
{
   struct mywork *mw = (struct mywork *)work;
   printf ("uri: %s\n", mw->uri);

   struct HttpResponse *response = uvllhttpd_response_init (mw->handle);
   response->status = 200;
   char contentType[] = "Content-Type: text/plain";
   uvllhttpd_response_add_header (response, contentType, sizeof(contentType)-1);

   char body[] = "Hello World";
   uvllhttpd_response_append_body (response, body, sizeof(body)-1);

   uvllhttpd_response_finish (response);
}

void after_work_cb (uv_work_t* work, int status)
{
   struct mywork *mw = (struct mywork *)work;
   free (mw);
}

void on_request3 (uv_tcp_t *handle, struct HttpRequest const *request)
{
   printf ("on_request3\n");

   struct mywork *mw = malloc (sizeof(struct mywork));
   mw->handle = handle;
   strncpy (mw->uri, request->uri.base, 100);
   
   assert (&(mw->work) == (uv_work_t *)mw);
   uv_queue_work (handle->loop, (uv_work_t *)mw, work_cb, after_work_cb);
}

int main (void)
{
   uv_loop_t *loop = uv_default_loop ();

   struct HttpServer server = {
      .loop = loop,
      .on_request = on_request3,
      .host = "127.0.0.1",
      //.host = "0.0.0.0",
      .port = 12345,
      .request_buffer_increase_unit = 0,
      .request_buffer_max_size = 10240,
   };

   int r;

   r = uvllhttpd_server_listen (&server);
   assert (r == 0);

   r = uv_run (loop, UV_RUN_DEFAULT);
   assert (r == 0);

   return r;
}
