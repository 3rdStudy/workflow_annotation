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

#ifndef _SLEEPREQUEST_H_
#define _SLEEPREQUEST_H_

#include "CommScheduler.h"
#include "Communicator.h"
#include "SubTask.h"

// 一个请求的属性有Task和Session,
// Task 掌管请求的分发和结束
// Session掌管请求的通信和处理
class SleepRequest : public SubTask, public SleepSession {
 public:
  SleepRequest(CommScheduler *scheduler) {
    LOG_TRACE("SleepRequest creator");
    this->scheduler = scheduler;
  }

 public:
  virtual void dispatch() {
    LOG_TRACE("SleepRequest dispatch");
    // 往poller 添加定时器,到时间了自动触发.
    if (this->scheduler->sleep(this) < 0) {
      // 若添加失败,则结束当前任务分发.
      // 添加失败的原因有两个,
      // 第一个就是duration设置错误,此时 errno 没有值.
      // 第二个就是 __poller_node 节点 malloc 失败.
      this->state = SS_STATE_ERROR;
      this->error = errno;
      this->subtask_done();
    }
  }

 protected:
  int state;
  int error;

 protected:
  CommScheduler *scheduler;

 protected:
  virtual void handle(int state, int error) {
    LOG_TRACE("SleepRequest handle");
    this->state = state;
    this->error = error;
    this->subtask_done();
  }
};

#endif
