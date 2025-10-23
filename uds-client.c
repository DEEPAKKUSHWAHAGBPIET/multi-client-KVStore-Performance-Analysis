#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/uds-demo.sock"
#define BUF_SIZE 1024

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/* Read all data from fd until no more available or buffer full */
static ssize_t read_all(int fd, char *buf, size_t cap)
{
    ssize_t total = 0;
    while (total < (ssize_t)cap)
    {
        ssize_t n = read(fd, buf + total, cap - (size_t)total);
        if (n > 0)
        {
            total += n;
            if (n < 128) // small read -> likely end of message
                break;
        }
        else if (n == 0)
        {
            break; // EOF
        }
        else
        {
            if (errno == EINTR) continue;
            return -1;
        }
    }
    return total;
}

int main(void)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        die("socket");

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        die("connect");

    printf("[client] Connected to server at %s\n", SOCKET_PATH);
    printf("Commands: SET <key> <value> | GET <key> | exit\n");

    char buf[BUF_SIZE];
    while (1)
    {
        printf("> ");
        if (!fgets(buf, sizeof(buf), stdin))
            break;

        if (strncmp(buf, "exit", 4) == 0)
            break;

        size_t len = strlen(buf);
        if (buf[len - 1] != '\n')
        {
            // ensure newline at end
            buf[len] = '\n';
            buf[len + 1] = '\0';
            len++;
        }

        if (write(fd, buf, len) < 0)
        {
            perror("write");
            break;
        }

        ssize_t n = read_all(fd, buf, sizeof(buf) - 1);
        if (n < 0)
        {
            perror("read");
            break;
        }

        buf[n] = '\0';
        printf("[server] %s", buf);
    }

    close(fd);
    printf("[client] Disconnected.\n");
    return 0;
}
