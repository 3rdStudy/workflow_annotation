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

#include "SubTask.h"

/**
 * @brief 不论是并行还是串行，任务的分发都是顺序的，只不过分发到不同的request
 * 如dns dispatch到WFRouterTask::dispatch，
 * http task dispatch到WFComplexClientTask::dispatch,进而Communicator::request
 * 但这个过程是通过ParallelTask::dispatch()顺序依次分发的
 * 另外从服务端接收到数据，放进msgqueue,网络线程池触发handle,每次都必须调用subtask_done，
 * 最终调用用户注册的回掉函数，如果是基于此，并行处理的主要机制其实就是利用queue的特性，多个线程调用各自的注册的回掉函数；
 * 可以看到subtask_done 里面调用的实际上就是done,只是多了些兄弟任务的分发和父任务的处理罢了
 */
void SubTask::subtask_done() {
  SubTask *cur = this;
  ParallelTask *parent;
  SubTask **entry;

  while (1) {
    // 任何的Task都是parellel的subTask
    parent = cur->parent;
    entry = cur->entry;
    // 执行这个任务本身的回调，一般是对资源的释放。
    cur = cur->done();  // 调用任务回调函数， 返回NULL, 则说明没有下个任务了。
    auto next = cur;
    if (next)  // 返回了串行的下一个task
    {
      next->parent = parent;
      next->entry = entry;
      if (parent) *entry = next;
      // 不同任务分发至不同的处理请求
      // 切换到排在后面的兄弟任务,执行兄弟任务的分发,然后break.
      // 这样做的原理是,任务都是顺序分发的,上一个任务分发完毕,就执行下一个.
      // 且分发完,就会执行subtask_done,在其中执行了当前任务的done后,又执行了下一个任务的分发
      next->dispatch();  

      // 如果没有下一个任务了，且父节点不为空,说明父节点的子任务执行完毕,
      // 则更新父节点的nleft状态,若父节点的剩下的子任务也为空,就执行父任务的substask_done.
      // 表明父节点没有子任务,该结束了.
      // TODO: 有没有可能出现next为空时,nleft减去1后不为0的情况?
    } else if (parent)   
    {
      if (__sync_sub_and_fetch(&parent->nleft, 1) == 0) {
        cur = parent;
        continue;
      }
    }

    break;
  }
}

// 并行任务的分发,就是依次调用所有子任务的分发,最后调用 subtask_done(),
// 表明子任务分发完毕
void ParallelTask::dispatch() {
  SubTask **end = this->subtasks + this->subtasks_nr;
  SubTask **p = this->subtasks;

  this->nleft = this->subtasks_nr;
  if (this->nleft != 0) {
    do {
      (*p)->parent = this;
      (*p)->entry = p;
      (*p)->dispatch();
    } while (++p != end);
  } else
    this->subtask_done();
}
