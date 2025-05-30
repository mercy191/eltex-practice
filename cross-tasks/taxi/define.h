#define FIFO_CLI_TO_DRIVER  "./tmp/taxi_cli_to_driver_%d"
#define FIFO_DRIVER_TO_CLI  "./tmp/taxi_driver_to_cli_%d"
#define CREATE_DRIVER       "Create_driver"
#define SEND_TASK           "Send_task"
#define GET_STATUS          "Get_status"
#define GET_DRIVERS         "Get_drivers"
#define TERMINATE           "Terminate"
#define EXIT                "Exit"

#define BUSY                "BUSY"
#define STATUS              "STATUS"
#define AVAILABLE           "AVAILABLE"
#define TASK_ACCEPTED       "TASK_ACCEPTED"
#define TASK_COMPLETE       "TASK_COMPLETE"
#define PING                "PING"
#define PONG                "PONG"
#define HEARTBEAT           "HEARTBEAT"

#define MAX_DRIVERS         10
#define BUFFER_SIZE         256
#define DRIVER_TIMEOUT      60
#define PATH_LEN            256
#define HEARTBEAT_INTERVAL  30