/*
  Copyright (c) 2019 Sogou, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Author: Xie Han (xiehan@sogou-inc.com)
*/

#ifndef _COMMSCHEDULER_H_
#define _COMMSCHEDULER_H_

#include <openssl/ssl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "Communicator.h"
#include "logger.h"

class CommSchedObject {
 public:
  size_t get_max_load() { return this->max_load; }
  size_t get_cur_load() { return this->cur_load; }

 private:
  // 这里是纯虚函数，他的两个子类 CommSchedTarget，CommSchedGroup 实现
  virtual CommTarget *acquire(int wait_timeout) = 0;

 protected:
  size_t max_load;
  size_t cur_load;

 public:
  virtual ~CommSchedObject() {}
  friend class CommScheduler;
};

class CommSchedGroup;

class CommSchedTarget : public CommSchedObject, public CommTarget {
 public:
  int init(const struct sockaddr *addr, socklen_t addrlen, int connect_timeout,
           int response_timeout, size_t max_connections);
  void deinit();

 public:
  int init(const struct sockaddr *addr, socklen_t addrlen, SSL_CTX *ssl_ctx,
           int connect_timeout, int ssl_connect_timeout, int response_timeout,
           size_t max_connections) {
    int ret = this->init(addr, addrlen, connect_timeout, response_timeout,
                         max_connections);

    if (ret >= 0) this->set_ssl(ssl_ctx, ssl_connect_timeout);

    return ret;
  }

 private:
  virtual CommTarget *acquire(int wait_timeout); /* final */
  virtual void release(int keep_alive);          /* final */

 private:
  CommSchedGroup *group;
  int index;
  int wait_cnt;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  friend class CommSchedGroup;
};

class CommSchedGroup : public CommSchedObject {
 public:
  int init();
  void deinit();
  int add(CommSchedTarget *target);
  int remove(CommSchedTarget *target);

 private:
  virtual CommTarget *acquire(int wait_timeout); /* final */

 private:
  CommSchedTarget **tg_heap;
  int heap_size;
  int heap_buf_size;
  int wait_cnt;
  pthread_mutex_t mutex;
  pthread_cond_t cond;

 private:
  static int target_cmp(CommSchedTarget *target1, CommSchedTarget *target2);
  void heapify(int top);
  void heap_adjust(int index, int swap_on_equal);
  int heap_insert(CommSchedTarget *target);
  void heap_remove(int index);
  friend class CommSchedTarget;
};

// 仅有一个成员变量Communicator
// 对于Communicator来说就是对外封装了一层
// 加入了一些逻辑操作，本质上都是comm的操作
class CommScheduler {
 public:
  int init(size_t poller_threads, size_t handler_threads) {
    LOG_TRACE("CommScheduler::init");
    return this->comm.init(poller_threads, handler_threads);
  }

  void deinit() { this->comm.deinit(); }

  /* wait_timeout in milliseconds, -1 for no timeout. */
  int request(CommSession *session, CommSchedObject *object, int wait_timeout,
              CommTarget **target) {
    LOG_TRACE("CommScheduler request");
    int ret = -1;
    // 就做两件事
    // 1. 获取target
    // 若 object 为对象，target 是当前对象，
    // 若 object 为对象组，target 是当前对象组的成员
    *target = object->acquire(wait_timeout);
    if (*target) {
      // 向target发送request
      ret = this->comm.request(session, *target);
      if (ret < 0) (*target)->release(0);
    }

    return ret;
  }

  /* for services. */
  int reply(CommSession *session) { return this->comm.reply(session); }

  int push(const void *buf, size_t size, CommSession *session) {
    return this->comm.push(buf, size, session);
  }

  // 这里的service为了产生sockfd
  int bind(CommService *service) { return this->comm.bind(service); }

  void unbind(CommService *service) { this->comm.unbind(service); }

  /* for sleepers. */
  int sleep(SleepSession *session) {
    LOG_TRACE("CommScheduler sleep");
    return this->comm.sleep(session);
  }

  /* for file aio services. */
  int io_bind(IOService *service) { return this->comm.io_bind(service); }

  void io_unbind(IOService *service) { this->comm.io_unbind(service); }

 public:
  int is_handler_thread() const { return this->comm.is_handler_thread(); }

  int increase_handler_thread() { return this->comm.increase_handler_thread(); }

 private:
  Communicator comm;

 public:
  virtual ~CommScheduler() {}
};

#endif
