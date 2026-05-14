/* ipc.c - 进程间通信实现 */

#include "ipc.h"
#include "process.h"
#include "allocator.h"
#include "syscall.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

/* 初始化 IPC 子系统 */
void ipc_init(void) {
    /* 目前不需要全局初始化，消息队列在进程创建时初始化 */
}

/* 分配消息结构 */
static message_t* alloc_message(void) {
    return (message_t*)kmalloc(sizeof(message_t));
}

/* 释放消息结构 */
static void free_message(message_t *msg) {
    if (msg != NULL) {
        kfree(msg);
    }
}

/* 将消息加入进程的消息队列 */
static void enqueue_message(struct process *proc, message_t *msg) {
    msg->next = NULL;

    if (proc->msg_queue_tail == NULL) {
        /* 队列为空 */
        proc->msg_queue = msg;
        proc->msg_queue_tail = msg;
    } else {
        /* 加入队列尾部 */
        proc->msg_queue_tail->next = msg;
        proc->msg_queue_tail = msg;
    }
}

/* 从进程的消息队列取出消息 */
static message_t* dequeue_message(struct process *proc, uint32_t from_pid) {
    message_t *msg = proc->msg_queue;
    message_t *prev = NULL;

    while (msg != NULL) {
        /* 检查是否匹配发送者 (from_pid=0 表示任意发送者) */
        if (from_pid == 0 || msg->sender_pid == from_pid) {
            /* 从队列移除 */
            if (prev == NULL) {
                proc->msg_queue = msg->next;
            } else {
                prev->next = msg->next;
            }

            /* 更新尾指针 */
            if (msg == proc->msg_queue_tail) {
                proc->msg_queue_tail = prev;
            }

            msg->next = NULL;
            return msg;
        }
        prev = msg;
        msg = msg->next;
    }

    return NULL;
}

/* 发送消息给目标进程 */
int ipc_send(uint32_t target_pid, uint32_t type, uint32_t data1, uint32_t data2) {
    struct process *sender = process_get_current();
    struct process *receiver = process_find_by_pid(target_pid);

    /* 检查目标进程是否存在 */
    if (receiver == NULL) {
        return IPC_NO_RECEIVER;
    }

    /* 检查目标进程是否在等待消息 */
    if (receiver->state == PROCESS_BLOCKED &&
        (receiver->waiting_for_sender == 0 ||
         receiver->waiting_for_sender == sender->pid)) {
        /* 目标进程正在等待，直接唤醒它 */
        receiver->waiting_for_sender = 0;
        process_unblock(receiver);
    }

    /* 创建消息 */
    message_t *msg = alloc_message();
    if (msg == NULL) {
        return IPC_ERROR;
    }

    msg->sender_pid = sender->pid;
    msg->receiver_pid = target_pid;
    msg->type = type;
    msg->data1 = data1;
    msg->data2 = data2;
    msg->next = NULL;

    /* 加入接收者的消息队列 */
    enqueue_message(receiver, msg);

    return IPC_SUCCESS;
}

/* 接收消息 (阻塞) */
int ipc_recv(uint32_t from_pid, message_t *msg) {
    struct process *receiver = process_get_current();

    /* 检查消息队列中是否有匹配的消息 */
    message_t *queued_msg = dequeue_message(receiver, from_pid);

    if (queued_msg != NULL) {
        /* 有消息，直接返回 */
        msg->sender_pid = queued_msg->sender_pid;
        msg->receiver_pid = queued_msg->receiver_pid;
        msg->type = queued_msg->type;
        msg->data1 = queued_msg->data1;
        msg->data2 = queued_msg->data2;
        msg->next = NULL;

        free_message(queued_msg);
        return IPC_SUCCESS;
    }

    /* 没有消息，阻塞等待 */
    receiver->waiting_for_sender = from_pid;
    process_block(receiver);
    schedule();

    /* 被唤醒后再次尝试接收 */
    queued_msg = dequeue_message(receiver, from_pid);
    if (queued_msg != NULL) {
        msg->sender_pid = queued_msg->sender_pid;
        msg->receiver_pid = queued_msg->receiver_pid;
        msg->type = queued_msg->type;
        msg->data1 = queued_msg->data1;
        msg->data2 = queued_msg->data2;
        msg->next = NULL;

        free_message(queued_msg);
        return IPC_SUCCESS;
    }

    return IPC_ERROR;
}

/* 非阻塞接收 */
int ipc_recv_nonblock(uint32_t from_pid, message_t *msg) {
    struct process *receiver = process_get_current();

    message_t *queued_msg = dequeue_message(receiver, from_pid);
    if (queued_msg != NULL) {
        msg->sender_pid = queued_msg->sender_pid;
        msg->receiver_pid = queued_msg->receiver_pid;
        msg->type = queued_msg->type;
        msg->data1 = queued_msg->data1;
        msg->data2 = queued_msg->data2;
        msg->next = NULL;

        free_message(queued_msg);
        return IPC_SUCCESS;
    }

    return IPC_NO_SENDER;
}

/* 发送消息并等待回复 (同步调用) */
int ipc_call(uint32_t target_pid, uint32_t type, uint32_t data1, uint32_t data2, message_t *reply) {
    /* 先发送消息 */
    int result = ipc_send(target_pid, type, data1, data2);
    if (result != IPC_SUCCESS) {
        return result;
    }

    /* 等待回复 */
    return ipc_recv(target_pid, reply);
}

/* 回复消息 */
int ipc_reply(uint32_t target_pid, uint32_t type, uint32_t data1, uint32_t data2) {
    return ipc_send(target_pid, type, data1, data2);
}