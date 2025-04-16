#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MQ_NAME             "/pingpong_mq"

#define PINGPONG_PRIORITY   10
#define STOP_PRIORITY       30
#define STOP_MSG            "stop"

#define MSG_BUF             100
#define STDIN_BUF           100
#define STDOUT_BUF          200

#define MAX_MSG             32

#define SLEEP_TIME          1
#define PERMISSIONS         (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

int main() {
    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MSG;
    attr.mq_msgsize = MSG_BUF;

    mqd_t mq;    
    if ((mq = mq_open(MQ_NAME, O_RDWR)) == -1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    char stdinbuf[STDIN_BUF];
    char stdoutbuf[STDOUT_BUF];
    char recvbuf[MSG_BUF];
    unsigned int priority;

    while (1) {
        
         /* Receive message from Ping*/
        if (mq_receive(mq, recvbuf, MSG_BUF, &priority) == -1) {
            perror("mq_receive");
            break;
        }

        /* If received message is STOP_PRIORITY then break*/
        if (priority == STOP_PRIORITY) {
            write_stdout("Pong stop.\n");
            break;
        } 
        /* If received message is PINGPONG_PRIORITY then print */
        else if (priority == PINGPONG_PRIORITY) {
            snprintf(stdoutbuf, sizeof(stdoutbuf), "Pong receive message: %s\n", recvbuf);
            write_stdout(stdoutbuf);
        }


        /*  Send message in Ping */

        /* Read stdin message */  
        write_stdout("Pong send message: ");
        if (fgets(stdinbuf, sizeof(stdinbuf), stdin) == NULL) {
            perror("fgets");
            break;
        }

        stdinbuf[strcspn(stdinbuf, "\n")] = '\0';
        priority = strcmp(stdinbuf, STOP_MSG) == 0 ? STOP_PRIORITY : PINGPONG_PRIORITY;

        /* Send stdin message */
        if (mq_send(mq, stdinbuf, strlen(stdinbuf) + 1, priority) == -1) {
            perror("mq_send");
            break;
        }

        /* If stdin message is STOP_PRIORITY then break*/
        if (priority == STOP_PRIORITY) {
            write_stdout("Ping stop.\n");
            break;
        }

        sleep(SLEEP_TIME);
    }

    mq_close(mq);
    exit(EXIT_SUCCESS);
}
