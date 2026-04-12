#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include "monitor_ioctl.h"

#define SOCK_PATH "/tmp/mini_runtime.sock"
#define MAX 10
#define STACK_SIZE (1024 * 1024)

struct container {
    char id[32];
    pid_t pid;
    int running;
};

static struct container containers[MAX];
static int count = 0;

static char child_stack[STACK_SIZE];

/* ==========================================================
   Container child process
   ========================================================== */
int child_func(void *arg)
{
    char *rootfs = (char *)arg;

    sethostname("alpha-container", 15);

    if (chroot(rootfs) < 0) {
        perror("chroot");
        return 1;
    }

    chdir("/");

    mkdir("/proc", 0555);

    if (mount("proc", "/proc", "proc", 0, NULL) < 0) {
        perror("mount proc");
    }

    execl("/bin/sh", "sh", NULL);

    perror("exec failed");
    return 1;
}

/* ==========================================================
   Register PID with kernel monitor
   ========================================================== */
void register_monitor(pid_t pid, char *id)
{
    int fd = open("/dev/container_monitor", O_RDWR);

    if (fd >= 0) {
        struct monitor_request req;

        memset(&req, 0, sizeof(req));

        req.pid = pid;
        strcpy(req.container_id, id);

        req.soft_limit_bytes = 20UL << 20;
        req.hard_limit_bytes = 40UL << 20;

        ioctl(fd, MONITOR_REGISTER, &req);
        close(fd);
    }
}

/* ==========================================================
   Reap dead children
   ========================================================== */
void reap_children()
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {

        for (int i = 0; i < count; i++) {
            if (containers[i].pid == pid) {
                containers[i].running = 0;
            }
        }
    }
}

/* ==========================================================
   Supervisor loop
   ========================================================== */
void run_supervisor()
{
    int server_fd, client_fd;
    struct sockaddr_un addr;

    unlink(SOCK_PATH);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCK_PATH);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("Supervisor running...\n");
    fflush(stdout);

    while (1) {

        reap_children();

        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0)
            continue;

        char buf[256] = {0};
        read(client_fd, buf, sizeof(buf));

        /* -------------------------------- START --------------------------- */
        if (strncmp(buf, "start", 5) == 0) {

            char id[32];
            char rootfs[64];

            sscanf(buf, "start %31s", id);

            snprintf(rootfs, sizeof(rootfs),
                     "./rootfs-%s", id);

            pid_t pid = clone(
                child_func,
                child_stack + STACK_SIZE,
                CLONE_NEWPID |
                CLONE_NEWUTS |
                CLONE_NEWNS |
                SIGCHLD,
                rootfs
            );

            if (pid < 0) {
                write(client_fd, "failed\n", 7);
                close(client_fd);
                continue;
            }

            register_monitor(pid, id);

            strcpy(containers[count].id, id);
            containers[count].pid = pid;
            containers[count].running = 1;
            count++;

            write(client_fd, "started\n", 8);
        }

        /* -------------------------------- PS ------------------------------ */
        else if (strncmp(buf, "ps", 2) == 0) {

            char out[1024] = "";

            for (int i = 0; i < count; i++) {

                char line[128];

                snprintf(line, sizeof(line),
                         "%s PID=%d RUNNING=%d\n",
                         containers[i].id,
                         containers[i].pid,
                         containers[i].running);

                strcat(out, line);
            }

            write(client_fd, out, strlen(out));
        }

        /* -------------------------------- STOP ---------------------------- */
        else if (strncmp(buf, "stop", 4) == 0) {

            char id[32];
            sscanf(buf, "stop %31s", id);

            for (int i = 0; i < count; i++) {

                if (strcmp(containers[i].id, id) == 0 &&
                    containers[i].running) {

                    kill(containers[i].pid, SIGKILL);
                    containers[i].running = 0;
                }
            }

            write(client_fd, "stopped\n", 8);
        }

        close(client_fd);
    }
}

/* ==========================================================
   Client helper
   ========================================================== */
void send_cmd(char *msg)
{
    int fd;
    struct sockaddr_un addr;
    char buf[1024] = {0};

    fd = socket(AF_UNIX, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCK_PATH);

    connect(fd, (struct sockaddr *)&addr, sizeof(addr));

    write(fd, msg, strlen(msg));
    read(fd, buf, sizeof(buf));

    printf("%s", buf);

    close(fd);
}

/* ==========================================================
   Main
   ========================================================== */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("usage:\n");
        printf("./engine supervisor\n");
        printf("./engine start alpha\n");
        printf("./engine ps\n");
        printf("./engine stop alpha\n");
        return 0;
    }

    if (strcmp(argv[1], "supervisor") == 0) {
        run_supervisor();
    }

    else if (strcmp(argv[1], "start") == 0) {

        char cmd[100];
        snprintf(cmd, sizeof(cmd),
                 "start %s", argv[2]);

        send_cmd(cmd);
    }

    else if (strcmp(argv[1], "ps") == 0) {
        send_cmd("ps");
    }

    else if (strcmp(argv[1], "stop") == 0) {

        char cmd[100];
        snprintf(cmd, sizeof(cmd),
                 "stop %s", argv[2]);

        send_cmd(cmd);
    }

    return 0;
}

