#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define SOCKET_PATH "/tmp/uds-demo.sock"
#define BACKLOG 10
#define BUF_SIZE 1024
#define MAX_KV_STORE 100

static int listen_fd = -1;

typedef struct
{
    char key[100];
    char value[200];
} KV_PAIR;

KV_PAIR hashMap[MAX_KV_STORE];
int hmsize = 0;

/*----------------------------------------------*/
static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/*----------------------------------------------*/
static void cleanup(void)
{
    if (listen_fd != -1)
        close(listen_fd);
    unlink(SOCKET_PATH);
}

/*----------------------------------------------*/
static void on_signal(int sig)
{
    if (sig == SIGCHLD)
    {
        // Reap zombie children
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
    }
    else
    {
        exit(0);
    }
}

/*----------------------------------------------*/
static ssize_t read_line(int fd, char *buf, size_t maxlen)
{
    size_t n = 0;
    while (n + 1 < maxlen)
    {
        char c;
        ssize_t r = read(fd, &c, 1);
        if (r == 0)
            break;
        if (r < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }

        buf[n++] = c;
        if (c == '\n')
            break;
    }
    buf[n] = '\0';
    return (ssize_t)n;
}

/*----------------------------------------------*/
static int write_all(int fd, const void *buf, size_t len)
{
    const char *p = buf;
    while (len > 0)
    {
        ssize_t w = write(fd, p, len);
        if (w < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        p += w;
        len -= (size_t)w;
    }
    return 0;
}

/*----------------------------------------------*/
static void set_value(const char *key, const char *value)
{
    for (int i = 0; i < hmsize; i++)
    {
        if (strcmp(hashMap[i].key, key) == 0)
        {
            strncpy(hashMap[i].value, value, sizeof(hashMap[i].value) - 1);
            return;
        }
    }
    if (hmsize < MAX_KV_STORE)
    {
        strncpy(hashMap[hmsize].key, key, sizeof(hashMap[hmsize].key) - 1);
        strncpy(hashMap[hmsize].value, value, sizeof(hashMap[hmsize].value) - 1);
        hmsize++;
    }
}

/*----------------------------------------------*/
static char *get_value(const char *key)
{
    for (int i = 0; i < hmsize; i++)
    {
        if (strcmp(hashMap[i].key, key) == 0)
            return hashMap[i].value;
    }
    return NULL;
}

/*----------------------------------------------*/
static void handle_client(int client_fd)
{
    char buf[BUF_SIZE];
    ssize_t n;

    while ((n = read_line(client_fd, buf, sizeof(buf))) > 0)
    {
        char cmd[16], key[256], value[256];
        cmd[0] = key[0] = value[0] = 0;

        if (sscanf(buf, "%15s %255s %255s", cmd, key, value) >= 2)
        {
            if (strcmp(cmd, "SET") == 0)
            {
                set_value(key, value);
                write_all(client_fd, "OK\n", 3);
            }
            else if (strcmp(cmd, "GET") == 0)
            {
                const char *val = get_value(key);
                if (val)
                    write_all(client_fd, val, strlen(val)), write_all(client_fd, "\n", 1);
                else
                    write_all(client_fd, "NOT_FOUND\n", 10);
            }
            else
            {
                write_all(client_fd, "ERROR: Unknown command\n", 23);
            }
        }
        else
        {
            write_all(client_fd, "ERROR: Invalid format\n", 23);
        }
    }

    close(client_fd);
    exit(0); // Child exits after handling one client
}

/*----------------------------------------------*/
int main(void)
{
    atexit(cleanup);

    struct sigaction sa = {0};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);

    umask(077);

    listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd == -1)
        die("socket(AF_UNIX)");

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCKET_PATH);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        die("bind");

    if (chmod(SOCKET_PATH, 0600) == -1)
        die("chmod");

    if (listen(listen_fd, BACKLOG) == -1)
        die("listen");

    fprintf(stderr, "[server] listening on %s\n", SOCKET_PATH);

    for (;;)
    {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0)
        {
            if (errno == EINTR)
                continue;
            die("accept");
        }

        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            close(client_fd);
            continue;
        }
        else if (pid == 0)
        {
            close(listen_fd); // Child doesn’t need listening socket
            handle_client(client_fd);
        }
        else
        {
            close(client_fd); // Parent doesn’t need client socket
        }
    }

    return 0;
}
