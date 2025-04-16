#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define MSG_KEY             5454

#define PINGPONG_PRIORITY   10
#define STOP_PRIORITY       30
#define STOP_MSG            "stop"

#define MSG_BUF             100
#define STDIN_BUF           100
#define STDOUT_BUF          200

#define SLEEP_TIME          1
#define PERMISSIONS         (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

typedef struct msg {
    long mtype;  
    char mtext[MSG_BUF];  
} msg;

void write_stdout(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

int main() 
{
    int msgid;
    if ((msgid = msgget(MSG_KEY, PERMISSIONS)) == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    msg message;
    char stdinbuf[STDIN_BUF];
    char stdoutbuf[STDOUT_BUF];


    while (1) {

        /* Received message from Ping */      
        if (msgrcv(msgid, &message, sizeof(message.mtext), -STOP_PRIORITY, 0) == -1) {
            perror("msgrcv");
            break;
        } 

        /* If received message is STOP_PRIORITY then break*/
        if (message.mtype == STOP_PRIORITY) {
            write_stdout("Pong stop.\n");
            break;
        }
        /* If received message is PINGPONG_PRIORITY then print */
        else if (message.mtype == PINGPONG_PRIORITY){
            snprintf(stdoutbuf, sizeof(stdoutbuf), "Pong receive message: %s\n", message.mtext);
            write_stdout(stdoutbuf);  
        }
        else {
            continue;
        }
        

        /* Send message in Ping */

        /* Read stdin message */
        write_stdout("Pong send message: ");
        if (fgets(stdinbuf, sizeof(stdinbuf), stdin) == NULL) {
            perror("fgets");
            break;
        }

        strcpy(message.mtext, stdinbuf);
        message.mtext[strcspn(message.mtext, "\n")] = '\0';
        if (strcmp(message.mtext, STOP_MSG) == 0){
            message.mtype = STOP_PRIORITY;
        }
        else {
            message.mtype = PINGPONG_PRIORITY;
        }    

        /* Send stdin message */
        if (msgsnd(msgid, &message, sizeof(message.mtext), 0) == -1) {
            perror("msgsnd");
            break;
        }

        /* If stdin message is STOP_PRIORITY then break*/
        if (message.mtype == STOP_PRIORITY) {
            write_stdout("Pong stop.\n");
            break;
        }

        sleep(SLEEP_TIME); 
    }

    exit(EXIT_SUCCESS);
}
