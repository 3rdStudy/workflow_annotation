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
 
#ifndef _COMMUNICATOR_H_
#define _COMMUNICATOR_H_

#include <openssl/ssl.h>
#include <pthread.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>

#include "list.h"
#include "logger.h"
#include "poller.h"

// 用来设置连接上下文的
class CommConnection {
 protected:
  virtual ~CommConnection() {}
  friend class Communicator;
};

// CommTarget是通讯目标，基本上就是ip+port,
// 还有两个超时参数。连接池什么的都在target里。
class CommTarget {
 public:
  int init(const struct sockaddr *addr, socklen_t addrlen, int connect_timeout,
           int response_timeout);
  void deinit();

 public:
  void get_addr(const struct sockaddr **addr, socklen_t *addrlen) const {
    *addr = this->addr;
    *addrlen = this->addrlen;
  }

  int get_connect_timeout() const { return this->connect_timeout; }
  int get_response_timeout() const { return this->response_timeout; }

 protected:
  void set_ssl(SSL_CTX *ssl_ctx, int ssl_connect_timeout) {
    this->ssl_ctx = ssl_ctx;
    this->ssl_connect_timeout = ssl_connect_timeout;
  }

  SSL_CTX *get_ssl_ctx() const { return this->ssl_ctx; }

 private:
  virtual int create_connect_fd() {
    return socket(this->addr->sa_family, SOCK_STREAM, 0);
  }

  virtual CommConnection *new_connection(int connect_fd) {
    return new CommConnection;
  }

  virtual int init_ssl(SSL *ssl) { return 0; }

 public:
  virtual void release(int keep_alive) {}

 private:
  struct sockaddr *addr;
  socklen_t addrlen;
  int connect_timeout;
  int response_timeout;
  int ssl_connect_timeout;
  SSL_CTX *ssl_ctx;
  // SSL_CTX数据结构主要用于SSL握手前的环境准备，设置CA文件和目录、设置SSL握手中的证书文件和私钥、设置协议版本以及其他一些SSL握手时的选项。
  // SSL_CTX中缓存了所有SSL_SESSION信息
  // 一般SSL_CTX的初始化在程序最开始调用
 private:
  struct list_head
      idle_list;  // idle_list本来是指keep-alive的可复用连接, 此处命名有歧意
  pthread_mutex_t mutex;

 public:
  virtual ~CommTarget() {}
  friend class CommSession;
  friend class Communicator;
};

/*
序列化函数encode

encode函数在消息被发送之前调用，每条消息只调用一次。

encode函数里，用户需要将消息序列化到一个vector数组，数组元素个数不超过max。目前max的值为8192。

结构体struct iovec定义在请参考系统调用readv和writev。

encode函数正确情况下的返回值在0到max之间，表示消息使用了多少个vector。

如果是UDP协议，请注意总长度不超过64k，并且使用不超过1024个vector（Linux一次writev只能1024个vector）。
UDP协议只能用于client，无法实现UDP server。

encode返回-1表示错误。返回-1时，需要置errno。如果返回值>max，将得到一个EOVERFLOW错误。错误都在callback里得到。
为了性能考虑vector里的iov_base指针指向的内容不会被复制。所以一般指向消息类的成员。
*/
class CommMessageOut {
 private:
  virtual int encode(struct iovec vectors[], int max) = 0;

 public:
  virtual ~CommMessageOut() {}
  friend class Communicator;
};

class CommMessageIn : private poller_message_t {
 private:
  virtual int append(const void *buf, size_t *size) = 0;

 protected:
  /* Send small packet while receiving. Call only in append(). */
  virtual int feedback(const void *buf, size_t size);

 private:
  struct CommConnEntry *entry;

 public:
  virtual ~CommMessageIn() {}
  friend class Communicator;
};

#define CS_STATE_SUCCESS 0
#define CS_STATE_ERROR 1
#define CS_STATE_STOPPED 2
#define CS_STATE_TOREPLY 3 /* for service session only. */

/*
CommSession是一次req->resp的交互，主要要实现message_in(),
message_out()等几个虚函数，让核心知道怎么产生消息。
对server来讲，session是被动产生的。即 session 是客户端和服务端通信的会话。
*/
class CommSession {
 private:
  virtual CommMessageOut *message_out() = 0;  // 往连接上要发的数据
  virtual CommMessageIn *
  message_in() = 0;  // 连接上收到数据流，如何切下一个数据包
  virtual int send_timeout() { return -1; }
  virtual int receive_timeout() { return -1; }
  virtual int keep_alive_timeout() { return 0; }
  virtual int first_timeout() { return 0; } /* for client session only. */
  virtual void handle(int state,
                      int error) = 0;  // 连接上收到数据流，如何切下一个数据包

 private:
  virtual int connect_timeout() { return this->target->connect_timeout; }
  virtual int response_timeout() { return this->target->response_timeout; }

 protected:
  CommTarget *get_target() const { return this->target; }
  CommConnection *get_connection() const { return this->conn; }
  CommMessageOut *get_message_out() const { return this->out; }
  CommMessageIn *get_message_in() const { return this->in; }
  long long get_seq() const { return this->seq; }

 private:
  CommTarget *target;
  CommConnection *conn;
  CommMessageOut *out;
  CommMessageIn *in;
  long long seq;

 private:
  struct timespec begin_time;
  int timeout;
  int passive;

 public:
  CommSession() { this->passive = 0; }
  virtual ~CommSession();
  friend class Communicator;
};

/*
CommService就是服务了，主要是new_session()的实现，因为对server来讲，session是被动产生的。
*
* 用来产生listenfd
* 产生新的连接
*/
class CommService {
 public:
 // 初始化配置参数，并不做创建网络通道的事情，值是为后面做准备
  int init(const struct sockaddr *bind_addr, socklen_t addrlen,
           int listen_timeout, int response_timeout);
  void deinit();

  int drain(int max);

 public:
  void get_addr(const struct sockaddr **addr, socklen_t *addrlen) const {
    *addr = this->bind_addr;
    *addrlen = this->addrlen;
  }

 protected:
  void set_ssl(SSL_CTX *ssl_ctx, int ssl_accept_timeout) {
    this->ssl_ctx = ssl_ctx;
    this->ssl_accept_timeout = ssl_accept_timeout;
  }

  SSL_CTX *get_ssl_ctx() const { return this->ssl_ctx; }

 private:
  virtual CommSession *new_session(long long seq, CommConnection *conn) = 0;
  virtual void handle_stop(int error) {}
  virtual void handle_unbound() = 0;

 private:
  virtual int create_listen_fd() {
    return socket(this->bind_addr->sa_family, SOCK_STREAM, 0);
  }

  virtual CommConnection *new_connection(int accept_fd) {
    return new CommConnection;
  }

  virtual int init_ssl(SSL *ssl) { return 0; }

 private:
  struct sockaddr *bind_addr;
  socklen_t addrlen;
  int listen_timeout;
  int response_timeout;
  int ssl_accept_timeout;
  SSL_CTX *ssl_ctx;

 public:
  void incref() { __sync_add_and_fetch(&this->ref, 1); }

  void decref() {
    if (__sync_sub_and_fetch(&this->ref, 1) == 0) this->handle_unbound();
  }

 private:
  int listen_fd;
  int ref;

 private:
  struct list_head alive_list;
  pthread_mutex_t mutex;

 public:
  virtual ~CommService() {}
  friend class CommServiceTarget;
  friend class Communicator;
};

#define SS_STATE_COMPLETE 0
#define SS_STATE_ERROR 1
#define SS_STATE_DISRUPTED 2

class SleepSession {
 private:
  virtual int duration(struct timespec *value) = 0;
  virtual void handle(int state, int error) = 0;

 public:
  virtual ~SleepSession() {}
  friend class Communicator;
};

#ifdef __linux__
#include "IOService_linux.h"
#else
#include "IOService_thread.h"
#endif

/*
用于通信用的类
*/
class Communicator {
 public:
  // 在init中create_epoll
  int init(size_t poller_threads, size_t handler_threads);
  void deinit();

  // Communicator::request(CommSession *session, CommTarget
  // *target)这个接口就可以实现一个异步的网络请求了
  int request(CommSession *session, CommTarget *target);
  int reply(CommSession *session);

  int push(const void *buf, size_t size, CommSession *session);

  int bind(CommService *service);
  void unbind(CommService *service);

  int sleep(SleepSession *session);

  int io_bind(IOService *service);
  void io_unbind(IOService *service);

 public:
  int is_handler_thread() const;
  int increase_handler_thread();

 private:
  struct __mpoller *mpoller;
  struct __msgqueue *queue;
  struct __thrdpool *thrdpool;
  int stop_flag;

 private:
  int create_poller(size_t poller_threads);

  int create_handler_threads(size_t handler_threads);

  int nonblock_connect(CommTarget *target);
  int nonblock_listen(CommService *service);

  struct CommConnEntry *launch_conn(CommSession *session, CommTarget *target);
  struct CommConnEntry *accept_conn(class CommServiceTarget *target,
                                    CommService *service);

  void release_conn(struct CommConnEntry *entry);

  void shutdown_service(CommService *service);

  void shutdown_io_service(IOService *service);

  int send_message_sync(struct iovec vectors[], int cnt,
                        struct CommConnEntry *entry);
  int send_message_async(struct iovec vectors[], int cnt,
                         struct CommConnEntry *entry);

  int send_message(struct CommConnEntry *entry);

  struct CommConnEntry *get_idle_conn(CommTarget *target);

  int request_idle_conn(CommSession *session, CommTarget *target);
  int reply_idle_conn(CommSession *session, CommTarget *target);

  void handle_incoming_request(struct poller_result *res);
  void handle_incoming_reply(struct poller_result *res);

  void handle_request_result(struct poller_result *res);
  void handle_reply_result(struct poller_result *res);

  void handle_write_result(struct poller_result *res);
  void handle_read_result(struct poller_result *res);

  void handle_connect_result(struct poller_result *res);
  void handle_listen_result(struct poller_result *res);

  void handle_ssl_accept_result(struct poller_result *res);

  void handle_sleep_result(struct poller_result *res);

  void handle_aio_result(struct poller_result *res);

  static void handler_thread_routine(void *context);

  static int first_timeout(CommSession *session);
  static int next_timeout(CommSession *session);

  static int first_timeout_send(CommSession *session);
  static int first_timeout_recv(CommSession *session);

  static int append(const void *buf, size_t *size, poller_message_t *msg);

  static int create_service_session(struct CommConnEntry *entry);

  static poller_message_t *create_message(void *context);

  static int partial_written(size_t n, void *context);

  static void callback(struct poller_result *res, void *context);

  static void *accept(const struct sockaddr *addr, socklen_t addrlen,
                      int sockfd, void *context);

 public:
  virtual ~Communicator() {}
};

#endif
