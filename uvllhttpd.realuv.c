#include <uv.h>


int real_uv_ip4_addr(const char* ip, int port, struct sockaddr_in* addr)
{
   return uv_ip4_addr(ip, port, addr);
}
