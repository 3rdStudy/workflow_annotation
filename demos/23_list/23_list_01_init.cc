#include "list.h"
#include "spdlog/spdlog.h"
#include "workflow/Communicator.h"

// init demo
struct CommConnEntry {
  struct list_head list;  // 用于串起来
  CommConnection *conn;
  long long seq;
  int sockfd;  // 每一个连接都是产生了个cfd的
#define CONN_STATE_CONNECTING 0
#define CONN_STATE_CONNECTED 1
#define CONN_STATE_RECEIVING 2
#define CONN_STATE_SUCCESS 3
#define CONN_STATE_IDLE 4
#define CONN_STATE_KEEPALIVE 5
#define CONN_STATE_CLOSING 6
#define CONN_STATE_ERROR 7
  int state;
  int error;
  int ref;
  struct iovec *write_iov;  // 写缓冲区
  SSL *ssl;
  CommSession *session;
  CommTarget *target;  // 对端的通信目标
  CommService *service;
  /* Connection entry's mutex is for client session only. */
  pthread_mutex_t mutex;
};

struct Task {
  struct list_head entry_list;
  int val;
};

int main() {
  struct Task task;
  INIT_LIST_HEAD(&task.entry_list);

  int diff = (unsigned long)(&((struct CommConnEntry *)0)->list);

  spdlog::info("diff: {}", diff);

  auto entry = (struct CommConnEntry
                    *)((char *)(&task.entry_list) -
                       (unsigned long)(&((struct CommConnEntry *)0)->list));
  return 0;
}
