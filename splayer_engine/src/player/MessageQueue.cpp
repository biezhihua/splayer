#include "player/MessageQueue.h"

MessageQueue::MessageQueue() {
    abortRequest = false;
    size = 0;
    firstMsg = nullptr;
    lastMsg = nullptr;
}

MessageQueue::~MessageQueue() {

}

void MessageQueue::start() {
    Mutex::Autolock lock(mutex);
    abortRequest = false;
    Message msg;
    message_init(&msg);
    msg.what = MSG_FLUSH;
}

void MessageQueue::stop() {
    Mutex::Autolock lock(mutex);
    abortRequest = true;
    condition.signal();
}

void MessageQueue::flush() {
    Message *msg, *msg1;
    Mutex::Autolock lock(mutex);
    for (msg = firstMsg; msg != nullptr; msg = msg1) {
        msg1 = msg->next;
        av_freep(&msg);
    }
    firstMsg = nullptr;
    lastMsg = nullptr;
    size = 0;
    condition.signal();
}

void MessageQueue::release() {
    flush();
}

void MessageQueue::notifyMessage(int what) {
    Message msg;
    message_init(&msg);
    msg.what = what;
    putMessage(&msg);
}

void MessageQueue::notifyMessage(int what, int arg1) {
    Message msg;
    message_init(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    putMessage(&msg);
}

void MessageQueue::notifyMsg(int what, int arg1, int arg2) {
    Message msg;
    message_init(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    msg.arg2 = arg2;
    putMessage(&msg);
}

void MessageQueue::notifyMessage(int what, int arg1, int arg2, void *obj, int len) {
    Message msg;
    message_init(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    msg.arg2 = arg2;
    msg.obj = av_malloc(sizeof(len));
    memcpy(msg.obj, obj, len);
    msg.free = message_free;
    putMessage(&msg);
}

int MessageQueue::getMessage(Message *msg) {
    return getMessage(msg, 1);
}

int MessageQueue::getMessage(Message *msg, int block) {
    Message *msg1;
    int ret;
    mutex.lock();
    for (;;) {
        if (abortRequest) {
            ret = -1;
            break;
        }
        msg1 = firstMsg;
        if (msg1) {
            firstMsg = msg1->next;
            if (!firstMsg) {
                lastMsg = nullptr;
            }
            size--;
            *msg = *msg1;
            msg1->obj = nullptr;
            av_free(msg1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            condition.wait(mutex);
        }
    }
    mutex.unlock();

    return ret;
}

void MessageQueue::removeMessage(int what) {
    Mutex::Autolock lock(mutex);
    Message **p_msg, *msg, *last_msg;
    last_msg = firstMsg;
    if (!abortRequest && firstMsg) {
        p_msg = &firstMsg;
        while (*p_msg) {
            msg = *p_msg;

            if (msg->what == what) {
                *p_msg = msg->next;
                av_free(msg);
                size--;
            } else {
                last_msg = msg;
                p_msg = &msg->next;
            }
        }

        if (firstMsg) {
            lastMsg = last_msg;
        } else {
            lastMsg = nullptr;
        }
    }
    condition.signal();
}

int MessageQueue::putMessage(Message *msg) {
    Mutex::Autolock lock(mutex);
    Message *message;
    if (abortRequest) {
        return -1;
    }
    message = (Message *) av_malloc(sizeof(Message));
    if (!message) {
        return -1;
    }
    *message = *msg;
    message->next = nullptr;

    if (!lastMsg) {
        firstMsg = message;
    } else {
        lastMsg->next = message;
    }
    lastMsg = message;
    size++;
    condition.signal();
    return 0;
}