#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <error.h>

#include <sys/socket.h> /* socket, connect */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <signal.h>
#include <poll.h>

#include <bstrlib.h>
#include <bstrlib_helper.h>
#include <map.h>

#include <loop_adapt_configuration.h>
#include <loop_adapt_parameter_value.h>

#include <loop_adapt_configuration_socket_ringbuffer.h>

static int loop_adapt_config_socket_sockfd = -1;
static bstring loop_adapt_config_socket_hostname;
static int loop_adapt_config_socket_port = 0;
static struct hostent * loop_adapt_config_socket_server = NULL;
static struct sockaddr_in loop_adapt_config_socket_server_addr;

int loop_adapt_config_socket_thread_init(Ringbuffer_t in, Ringbuffer_t out)
{

    int err = 0;
    int sock_timeout = 10000;
    char* envhost = getenv("LA_CONFIG_SOCKET_HOST");
    char* host = ((envhost && strlen(envhost) > 0) ? envhost : "localhost");

    bstring x = bfromcstr(host);
    struct bstrList* xl = bsplit(x, ':');
    if (xl->qty == 1)
    {
        loop_adapt_config_socket_port = 12121;
        loop_adapt_config_socket_hostname = bstrcpy(x);
    }
    else
    {
        loop_adapt_config_socket_hostname = bstrcpy(xl->entry[0]);
        loop_adapt_config_socket_port = atoi(bdata(xl->entry[1]));
    }
    bstrListDestroy(xl);
    bdestroy(x);

    loop_adapt_config_socket_server = gethostbyname(bdata(loop_adapt_config_socket_hostname));
    if (!loop_adapt_config_socket_server)
    {
        bdestroy(loop_adapt_config_socket_hostname);
        ERROR_PRINT(No such host %s, bdata(loop_adapt_config_socket_hostname));
        return -EHOSTUNREACH;
    }

    loop_adapt_config_socket_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (loop_adapt_config_socket_sockfd < 0)
    {
        bdestroy(loop_adapt_config_socket_hostname);
        ERROR_PRINT(Failed to open socket);
        return loop_adapt_config_socket_sockfd;
    }

    memset(&loop_adapt_config_socket_server_addr, 0, sizeof(struct sockaddr_in));
    loop_adapt_config_socket_server_addr.sin_family = AF_INET;
    loop_adapt_config_socket_server_addr.sin_port = htons(loop_adapt_config_socket_port);
    memcpy(&loop_adapt_config_socket_server_addr.sin_addr.s_addr,
            loop_adapt_config_socket_server->h_addr_list[0],
            loop_adapt_config_socket_server->h_length);

    setsockopt(loop_adapt_config_socket_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&sock_timeout, sizeof(sock_timeout));
    err = connect(loop_adapt_config_socket_sockfd,(struct sockaddr *)&loop_adapt_config_socket_server_addr, sizeof(struct sockaddr_in));
    if (err < 0)
    {
        bdestroy(loop_adapt_config_socket_hostname);
        ERROR_PRINT(Failed to connect to %s:%d, bdata(loop_adapt_config_socket_hostname), loop_adapt_config_socket_port);
        return err;
    }


    return 0;
}


void loop_adapt_config_socket_thread_finalize()
{
    close(loop_adapt_config_socket_sockfd);
    loop_adapt_config_socket_sockfd = -1;
}
