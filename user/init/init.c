#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    printf("Hello MUSL!\n");

    pid_t parent_pid = getpid();
    printf("[PARENT] PID is: %d\n", parent_pid);
    printf("[PARENT] Forking a child process...\n");

    pid_t pid = fork();

    if (pid < 0) {
        perror("[PARENT] fork failed");
        return 1;
    } 
    else if (pid == 0) {
        pid_t child_pid = getpid();
        printf("[CHILD] fork() returned: %d\n", pid);
        printf("[CHILD] My actual getpid() is: %d\n", child_pid);
        
        for (int i = 1; i <= 3; i++) {
            printf("[CHILD] Loop count: %d\n", i);
        }

        printf("[CHILD] Now calling execve(\"/bin/hello\", ...)\n");
        char *child_argv[] = {"/bin/hello", "hello_arg1", "hello_arg2", NULL};
        char *child_envp[] = {"PATH=/bin", "USER=heebb", "SHELL=/bin/sh", NULL};

        execve("/bin/hello", child_argv, child_envp);

        perror("[CHILD] execve FAILED");
        exit(1);
    } 
    else {
        printf("[PARENT] fork() returned (Child PID): %d\n", pid);
        
        for (volatile unsigned long long i = 0; i < 40000000ULL; i++);

        printf("[PARENT] 에이 뭐 대충 지금쯤 자식 끝났겠죠? 나중에합시다\n");
    }

    return 0;
}