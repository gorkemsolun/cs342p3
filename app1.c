#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <time.h>
#include "mf.h"

#define COUNT 2 // 10 in original code
char* semname1 = "/semaphore1";
char* semname2 = "/semaphore2";
sem_t* sem1, * sem2;
char* mqname1 = "msgqueue1";

int main(int argc, char** argv) {
    int ret, qid;
    // char sendbuffer[MAX_DATALEN]; // original code
    char sendbuffer[] = "Hello, World!AABBCCDDEEFFGGHHIIUUYYTTHHNNMMOOKKLLPPCCVVDDSSAAQQWWEE11223344556677889900--zzxxccvvbbnnmm<<TTGGHHYYUUJJKKIIOOLLPPMMNNBBVVCCXXZZAASSDDFFGGHHYYTTRREEWWQQ"; // not in original code
    int sendbuffer_len = strlen(sendbuffer); // not in original code
    int n_sent, n_received;
    char recvbuffer[MAX_DATALEN];
    int sentcount = 0;
    int receivedcount;
    int totalcount = COUNT;

    if (argc == 2)
        totalcount = atoi(argv[1]);

    sem1 = sem_open(semname1, O_CREAT, 0666, 0); // init sem
    sem2 = sem_open(semname2, O_CREAT, 0666, 0); // init sem

    srand(time(0));
    printf("RAND_MAX is %d\n", RAND_MAX);

    ret = fork();
    if (ret > 0) {
        // parent process - P1
        // parent will create a message queue

        mf_connect(); // in original code

        mf_create(mqname1, 16); //  create mq;  16 KB

        qid = mf_open(mqname1);

        sem_post(sem1);

        while (1) {
            // n_sent = rand() % MAX_DATALEN; // original code
            n_sent = rand() % sendbuffer_len; // not in original code
            ret = mf_send(qid, (void*)sendbuffer, n_sent);
            printf("app sent message, datalen=%d\n", n_sent);
            printf("Message: %s\n", sendbuffer); // not in original code, prints entire buffer
            sentcount++;

            mf_print();

            if (sentcount == totalcount)
                break;
        }
        mf_close(qid);
        sem_wait(sem2);
        // we are sure other process received the messages

        mf_remove(mqname1);   // remove mq
        mf_disconnect();
    } else if (ret == 0) {
        // child process - P2
        // child will connect, open mq, use mq
        sem_wait(sem1);
        // we are sure mq was created

        mf_connect(); // in original code

        qid = mf_open(mqname1);

        while (1) {
            n_received = mf_recv(qid, (void*)recvbuffer, MAX_DATALEN);
            printf("app received message, datalen=%d\n", n_received);
            printf("Message: %s\n", recvbuffer); // not in original code, prints the buffer, prints maximum so far
            receivedcount++;

            //mf_print();

            if (receivedcount == totalcount)
                break;
        }
        mf_close(qid);
        mf_disconnect();
        sem_post(sem2);
    }

    // Remove semaphores
    sem_unlink(semname1);
    sem_unlink(semname2);

    return 0;
}
