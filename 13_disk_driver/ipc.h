/* ipc.h - 进程间通信头文件 */

#ifndef IPC_H
#define IPC_H

#include <stdint.h>

/* IPC 返回值 */
#define IPC_SUCCESS     0
#define IPC_ERROR      -1
#define IPC_NO_SENDER  -2
#define IPC_NO_RECEIVER -3
#define IPC_INVALID_PID -4

/* 消息类型定义 */
#define MSG_TYPE_NONE       0
#define MSG_TYPE_DATA       1   /* 普通数据消息 */
#define MSG_TYPE_SIGNAL     2   /* 信号消息 */
#define MSG_TYPE_REQUEST    3   /* 请求消息 */
#define MSG_TYPE_REPLY      4   /* 回复消息 */

/* 消息结构 (24 字节) */
typedef struct message {
    uint32_t sender_pid;        /* 发送者 PID */
    uint32_t receiver_pid;      /* 接收者 PID */
    uint32_t type;              /* 消息类型 */
    uint32_t data1;             /* 数据字段 1 */
    uint32_t data2;             /* 数据字段 2 */
    struct message *next;       /* 链表指针 */
} message_t;

/* 初始化 IPC 子系统 */
void ipc_init(void);

/* 发送消息给目标进程 */
int ipc_send(uint32_t target_pid, uint32_t type, uint32_t data1, uint32_t data2);

/* 接收消息 (阻塞，from_pid=0 表示接收任意发送者的消息) */
int ipc_recv(uint32_t from_pid, message_t *msg);

/* 非阻塞接收 */
int ipc_recv_nonblock(uint32_t from_pid, message_t *msg);

/* 发送消息并等待回复 (同步调用) */
int ipc_call(uint32_t target_pid, uint32_t type, uint32_t data1, uint32_t data2, message_t *reply);

/* 回复消息 */
int ipc_reply(uint32_t target_pid, uint32_t type, uint32_t data1, uint32_t data2);

#endif
