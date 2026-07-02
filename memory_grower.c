#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <mcheck.h>

#define MB (1024 * 1024)
#define MAX_SHM_BLOCKS (2 * 1024 * 1024 / MB) // 2MB worth of 1MB blocks = 2 blocks

void run_memory_grower(const char *fifo_path) {
    char **memory_blocks = NULL;
    size_t block_count = 0;
    int shmids[MAX_SHM_BLOCKS];
    size_t shm_block_count = 0;

    // Create FIFO if it doesn't exist
    if (mkfifo(fifo_path, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(1);
    }

    // Open FIFO for reading (non-blocking)
    int fd = open(fifo_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open FIFO");
        exit(1);
    }

    printf("[PID %d] Memory grower started for FIFO: %s\n", getpid(), fifo_path);
    fflush(stdout);

    char buffer[64];
    while (1) {
        ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            char *newline = strchr(buffer, '\n');
            if (newline) *newline = '\0';

            // Handle shared memory commands: S+N or S-N
            if ((buffer[0] == 'S') && (buffer[1] == '+' || buffer[1] == '-') && strlen(buffer) > 2) {
                int mb = atoi(buffer + 2);
                if (buffer[1] == '+') {
                    for (int i = 0; i < mb; i++) {
                        if (shm_block_count >= MAX_SHM_BLOCKS) {
                            printf("[%d] SHM block array full\n", getpid());
                            break;
                        }
                        int shmid = shmget(IPC_PRIVATE, MB, IPC_CREAT | 0600);
                        if (shmid < 0) {
                            perror("shmget");
                            break;
                        }
                        void *shmptr = shmat(shmid, NULL, 0);
                        if (shmptr == (void *)-1) {
                            perror("shmat");
                            shmctl(shmid, IPC_RMID, NULL);
                            break;
                        }
                        memset(shmptr, 1, MB);
                        shmids[shm_block_count++] = shmid;
                        printf("[%d] [+] Allocated 1MB SHMEM (Total: %zu)\n", getpid(), shm_block_count);
                        fflush(stdout);
                    }
                } else {
                    for (int i = 0; i < mb && shm_block_count > 0; i++) {
                        int shmid = shmids[shm_block_count - 1];
                        void *shmptr = shmat(shmid, NULL, 0);
                        if (shmptr != (void *)-1) shmdt(shmptr);
                        shmctl(shmid, IPC_RMID, NULL);
                        shm_block_count--;
                    }
                    printf("[%d] [-] Freed %d SHMEM MB (Total: %zu)\n", getpid(), mb, shm_block_count);
                    fflush(stdout);
                }
            }
            // Handle heap memory commands: +N or -N
            else if ((buffer[0] == '+' || buffer[0] == '-') && strlen(buffer) > 1) {
                int mb = atoi(buffer + 1);

                if (buffer[0] == '+') {
                    // Allocate memory
                    for (int i = 0; i < mb; i++) {
                        char *block = malloc(MB);
                        if (!block) {
                            perror("malloc");
                            break;
                        }
                        memset(block, 1, MB); // Touch the memory
                        char **tmp = realloc(memory_blocks, (block_count + 1) * sizeof(char *));
                        if (!tmp) {
                            perror("realloc");
                            free(block);
                            break;
                        }
                        memory_blocks = tmp;
                        memory_blocks[block_count++] = block;
                    }
                    printf("[%d] [+] Allocated %d MB (Total: %zu)\n", getpid(), mb, block_count);
                    fflush(stdout);
                } else {
                    // Free memory
                    for (int i = 0; i < mb && block_count > 0; i++) {
                        free(memory_blocks[block_count - 1]);
                        block_count--;
                    }
                    char **tmp = realloc(memory_blocks, block_count * sizeof(char *));
                    if (tmp || block_count == 0) {
                        memory_blocks = tmp;
                    }
                    printf("[%d] [-] Freed %d MB (Total: %zu)\n", getpid(), mb, block_count);
                    fflush(stdout);
                }
            }
        }
        usleep(100000); // 100ms
    }

    // Cleanup heap memory
    for (size_t i = 0; i < block_count; i++) {
        free(memory_blocks[i]);
    }
    free(memory_blocks);

    // Cleanup shared memory
    while (shm_block_count > 0) {
        int shmid = shmids[shm_block_count - 1];
        void *shmptr = shmat(shmid, NULL, 0);
        if (shmptr != (void *)-1) shmdt(shmptr);
        shmctl(shmid, IPC_RMID, NULL);
        shm_block_count--;
    }

    close(fd);
    unlink(fifo_path);
}

static void maybe_start_mtrace(void) {
    // Enable tracing only if MALLOC_TRACE is set (so you can toggle per run)
    const char *mt = getenv("MALLOC_TRACE");
    if (mt && *mt) {
        mtrace();
        fprintf(stderr, "[%d] mtrace enabled, MALLOC_TRACE=%s\n", getpid(), mt);
    }
}

int main() {
    const char *fifo_path = "/tmp/mem_grower_single";

    //setenv("MALLOC_TRACE", "/tmp/mtrace.memory_grower.log", 1);
    maybe_start_mtrace();

    run_memory_grower(fifo_path);

    return 0;
}
