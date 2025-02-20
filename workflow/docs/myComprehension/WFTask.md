## WFTask.h

### WFT_STATE_ *

task 的各种状态

### WFThreadTask

> 继承 ExecRequest， 需要传入 class INPUT, class OUTPUT 两个模板参数

public functions :
* start: 开始任务 
* dismiss: 中止任务 
* get_input: 获取接收对象 
* get_output: 获取发送对象 
* get_state: 获取task 的状态 
* get_error: 获取当前的错误 
* set_callback: 设置任务的回调，任务快结束的时候执行 
* WFThreadTask: 当前对象的构造， 需要接收 ExecQueue， Executor， std::function 三个参数。

protected functions : 
* [virtual] ~WFThreadTask(): 限制对象只能通过 dismiss 来销毁，而不能自动释放
* [virtual] done: 任务结束，并销毁。（先执行上面设置的 callback, 再delete当前对象, 最后将本对象从当前 serial 中 pop 出去）

protected members:
* INPUT *input: 接收消息的对象 
* OUTPUT * output: 发送消息的对象 
* callback: task 的回调

### WFMultiThreadTask

> 继承 ParallelTask，需要传入 class INPUT, class OUTPUT 两个模板参数

public functions: 
* start: 开始任务
* dismiss: 中止任务
* get_input: 获取接收对象 
* get_output: 获取发送对象
* get_state: 获取task 的状态 
* get_error: 获取当前的错误 
* set_callback: 设置任务的回调，任务快结束的时候执行
* WFMultiThreadTask: 当前对象的构造， 需要接收 Thread *const tasks[]， size_t n， std::function三个参数。

protected functions:
* [virtual] ~WFMultiThreadTask(): 限制对象只能通过 dismiss 来销毁，而不能自动释放 
* [virtual] done: 任务结束，并销毁。（先执行上面设置的 callback,再delete当前对象,最后将本对象从当前 serial 中 pop 出去）

protected members: 
* callback: task 的回调

public members: 
* void *user_data: 当前task储存的用户数据

### WFNetworkTask

> 继承 CommRequest，需要传入class REQ, class RESP两个参数

public functions: 
* start: 开始任务
* dismiss: 中止任务 
* get_req: 获取 http 的请求方 
* get_resp: 获取 http 的回复方
* get_state: 获取task 的状态 
* get_error: 获取当前的错误 
* get_timeout_reason: 获取超时的原因
* get_task_seq: 仅仅在 server's process 或者 callback 调用
* get_peer_addr: 获取对端的网络详细信息, 内部使用 CommTarget 实现。
* [virtual] get_connection: 获取连接的上下文
* set_send_timeout: 设置发送超时
* set_receive_timeout: 设置接收超时 
* set_keep_alive: 设置连接超时
* noreply: 仅仅可以在 server 端使用，代表不再回复消息了 
* push: 仅仅可以在 server 端使用， 代表推送消息 
* set_callback: 设置任务的回调，任务快结束的时候执行 
* WFNetworkTask: 当前对象的构造， 需要接收 Thread *const tasks[]， size_t n， std::function 三个参数。

protected functions: 
* [virtual] send_timeout: 获取发送超时 
* [virtual] receive_timeout: 获取接收超时 
* [virtual] keep_alive_timeout: 获取连接超时 
* [virtual] done: 任务结束，并销毁。（先执行上面设置的 callback,再delete当前对象,最后将 本对象从当前 serial 中 pop 出去） 
* [virtual] ~WFNetworkTask(): 限制对象只能通过 dismiss 来销毁，而不能自动释放

protected members: 
* int send_timeo: 发送超时
* int receive_timeo: 接收超时
* int keep_alive_timeo: 连接超时 
* REQ req : http 请求对象 
* RESP resp : http 回复对象 
* callback: task 的回调

public members: 
* void *user_data: 当前task储存的用户数据

### WFTimerTask

> 继承 SleepRequest

public functions: 
* start: 开始任务
* dismiss: 中止任务 
* get_state: 获取task 的状态 
* get_error: 获取当前的错误 
* set_callback: 设置任务的回调，任务快结束的时候执行
* WFTimerTask: 当前对象的构造，需要接收 CommSchedObject，std::function 三个参数。

protected functions: 
* [virtual] done: 任务结束，并销毁。（先执行上面设置的 callback,再delete当前对象,最后将 本对象从当前 serial 中 pop 出去） 
* [virtual] ~WFTimerTask(): 限制对象只能通过 dismiss 来销毁，而不能自动释放

protected members: 
* callback: task 的回调

public members: 
* void *user_data: 当前task储存的用户数据

### WFFileTask

> 继承 IORequest，需要传入 class ARGS 一个参数。

public functions: 
* start: 开始任务
* dismiss: 中止任务 
* get_args: 获取文件的属性 
* get_retval: 获取返回值，参测是用于写入或读取的字节数
* get_state: 获取task 的状态 
* get_error: 获取当前的错误 
* set_callback: 设置任务的回调，任务快结束的时候执行
* WFFileTask: 当前对象的构造，需要接收 IOService，std::function 三个参数。

protected functions: 
* [virtual] done: 任务结束，并销毁。（先执行上面设置的 callback,再delete当前对象,最后将本对象从当前 serial 中 pop 出去） 
* [virtual] ~WFFileTask(): 限制对象只能通过 dismiss 来销毁，而不能自动释放

protected members: 
* ARGS args: 文件的属性
* callback: task 的回调

public members: 
* void * user_data: 当前task储存的用户数据

### WFGenericTask

> 继承 SubTask

public functions: 
* start: 开始任务
* dismiss: 中止任务 
* get_state: 获取task 的状态 
* get_error: 获取当前的错误 
* WFGenericTask: 当前对象的构造。

protected functions: 
* [virtual] dispatch: 任务的分发，派遣 
* [virtual] done: 任务结束，并销毁。（先执行上面设置的 callback,再delete当前对象,最后将 本对象从当前 serial 中 pop 出去） 
* [virtual] ~WFGenericTask(): 限制对象只能通过 dismiss 来销毁，而不能自动释放

protected members: 
* int state: 当前状态
* int error: 当前 error

public members: 
* void *user_data: 当前task储存的用户数据

### WFCounterTask

> 继承 WFGenericTask

public functions: 
* [virtual] count: 当前对象调用计数
* set_callback: 设置 task 的回调 
* WFCounterTask: 当前对象的构造，需要接受 unsigned int target_value,std::function 参数。

protected functions: 
* [virtual] dispatch: 任务的分发，派遣 
* [virtual] done: 任务结束，并销毁。（先执行上面设置的 callback,再delete当前对象,最后将 本对象从当前 serial 中 pop 出去） 
* [virtual] ~WFCounterTask(): 限制对象只能通过 dismiss 来销毁，而不能自动释放

protected members: 
* std::atomic<unsigned int> value: 目标值（原子变量）
* callback: 回调函数

### WFMailboxTask

> 继承 WFGenericTask

public functions: 
* send: 发送信息
* get_mailbox: 获取 mail 会话。 
* set_callback: 设置回调 
* [virtual] count: 当前原子值计数 
* WFMailboxTask: 构造函数，需传入 mailbox,size,std::function 参数。 
* WFMailboxTask: 构造函数，需传入 std::function 参数。

protected functions: 
* [virtual] dispatch: 任务的分发，派遣 
* [virtual] done: 任务结束，并销毁。（先执行上面设置的 callback,再delete当前对象,最后将 本对象从当前 serial 中 pop 出去） 
* ~WFMailboxTask: 限制对象只能通过 dismiss 来销毁，而不能自动释放

protected members: 
* void **mailbox: 邮件会话
* std::atomic<void **> next: 下个会话 
* std::atomic<size_t> value: 发送内容的大小
* std::function<void(WFMailboxTask *)> callback: 回调函数

### WFConditional

> 继承 WFGenericTask

public functions: 
* [virtual] signal: 发送激活信号。
* WFConditional: 构造函数，需要传入 task， msgbuf 参数。
* WFConditional: 构造函数， 需要传入 task 参数。

protected functions: 
* [virtual] dispatch: 任务的分发，派遣

protected members: 
* std::atomic<bool> flag: 标志位
* SubTask * task: 监听的任务对象 
* void **msgbuf: 监听的消息数据

### WFGoTask

> 继承 ExecRequest，和 WFThreadTask 为兄弟类。

public functions: 
* start: 开始任务
* dismiss: 中止任务 
* get_state: 获取task 的状态 
* get_error: 获取当前的错误 
* set_callback: 设置任务的回调，任务快结束的时候执行
* WFGoTask: 当前对象的构造， 需要接收 ExecQueue， Executor 参数。

protected functions: 
* [virtual] done: 任务结束，并销毁。（先执行上面设置的 callback,再delete当前对象,最后将 本对象从当前 serial 中 pop 出去） 
* [virtual] ~WFGoTask(): 限制对象只能通过 dismiss 来销毁，而不能自动释放

protected members: 
* callback: task 的回调

public members: 
* void *user_data: 当前task储存的用户数据


### WFClientTask

> 继承 WFNetworkTask

protected functions:
* [virtual] message_out: 获取 req, 且会先执行 prepare 函数。
* [virtual] message_in: 获取 resp。
* [virtual] get_connection: 获取连接上下文，通过 CommSession::get_connection 实现。
* [virtual] ～WFClientTask(): 限制对象只能通过 dismiss 来销毁，而不能自动释放 

public functions:
* set_prepare: 设置预处理函数
* WFClientTask: 构造函数，需传入 CommSchedObject,CommScheduler,std::function函数。

protected members:
* std::function<void(WFNetworkTask<REQ, RESP> *)> prepare: 预处理函数


### WFServerTask

> 继承 WFNetworkTask

protected functions:
* [virtual] message_out: 获取 req。
* [virtual] message_in: 获取 resp。
* [virtual] handle: 根据 state 的不同，执行不同的操作，要么继续派发，要么提前结束。
* [virtual] dispatch: 任务的分发，派遣
* [virtual] get_connection: 获取连接的上下文，通过 CommSession::get_connection 实现。
* [virtual] ~WFServerTask: 保护虚析构函数。

protected members:
* CommService *service: 服务
* Processor 类
  > 继承于 SubTask
  * public functions:
    * Processor: 构造参数，需要 WFServerTask, std::function 参数
    * [virtual] dispatch: 任务的分发，派遣
    * [virtual] done: 将本对象从当前 serial 中 pop 出去）
  * public memebers:
    * std::function<void(WFNetworkTask<REQ, RESP> *)> &process: 执行任务的函数
    * WFServerTask<REQ, RESP> *task: 关联的任务

* Series 类 
  > 继承于 SeriesWork
  * public functions:
    * Series: 构造参数，需要 WFServerTask 参数
    * [virtual] ~Series(): 析构函数
  * public memebers:
    * CommService *service: 服务

public functions:
* WFServerTask: 构造函数，需要 CommService，CommScheduler，std::function参数

