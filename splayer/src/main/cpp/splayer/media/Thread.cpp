
#include "Thread.h"

static void *runThread(void *data) {
    Thread *thread = static_cast<Thread *>(data);
    if (thread) {
        pthread_setname_np(pthread_self(), thread->name->c_str());
        thread->retval = thread->func(thread->data);
    }
    return NULL;
}

Thread::Thread(int (*func)(void *), void *data, const char *name) {
    Thread::func = func;
    Thread::data = data;
    Thread::name = new string(name);
    Thread::retval = pthread_create(&id, NULL, runThread, this);
}

Thread::~Thread() {
    delete name;
}

int Thread::waitThread() {
    pthread_join(id, NULL);
    return retval;
}

void Thread::detachThread() {
    pthread_detach(id);
}