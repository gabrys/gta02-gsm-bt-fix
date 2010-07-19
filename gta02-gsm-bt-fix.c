/*
 *
 * Invocation:
 *
 * gta02-gsm-bt-fix (start|stop) [pidfile]
 *
 */

#include <stdio.h>
#include <signal.h>

#include "common.h"

#define DEFAULT_PIDFILE "/var/run/gta02-gsm-bt-fix.pid"

void help() {
    fprintf(stderr, "gta02-gsm-bt-fix --help\n");
    fprintf(stderr, "gta02-gsm-bt-fix (start|stop) [pidfile]\n");
}

int stop_pid(int pid) {
    if (kill(pid, SIGTERM) != 0) {
        perror("Kill");
        return 1;
    }
    while (kill(pid, SIGTERM) == 0) {
        /* successful killing, try again */
        sleep(1);
    }
    /* killing unsuccessful -- process finally died */
    return 0;
}

int stop(char *pidfile) {
    FILE *f;
    int pid;
    
    if ((f = fopen(pidfile, "r")) == 0) {
        perror("Open pid file");
        return -1;
    }
    fscanf(f, "%d", &pid);
    return stop_pid(pid);
}

int start(char *pidfile) {
    int pid;
    FILE *f;

    pid = gsm_bt_enable();
    if (pid > 0) {
        if ((f = fopen(pidfile, "w")) == 0) {
            perror("Open pid file");
            fprintf(stderr, "Canceling gsm bt fix\n");
            stop_pid(pid);
            return 1;
        }
        fprintf(f, "%d\n", pid);
        fclose(f);
    } else if (pid < 0) {
        return -pid;
    }
    return 0;
}

int main(int argc, char **argv) {
    char *action;
    char *pidfile;

    if (argc >= 2) {
        action = argv[1];
    } else {
        action = "";
    }
    
    if (argc >= 3) {
        pidfile = argv[2];
    } else {
        pidfile = DEFAULT_PIDFILE;
    }

    if (strcmp(action, "start") == 0) {
        return start(pidfile);
    }
    if (strcmp(action, "stop") == 0) {
        return stop(pidfile);
    }
    help();
    if (strcmp(action, "--help") == 0) {
        return 0;
    }
    return 1;
}

