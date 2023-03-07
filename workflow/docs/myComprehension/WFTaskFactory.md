# WFTaskFactory

> 包含 WFTaskFactory.h WFTaskFactory.inl 两个文件

## WFNetworkTask

此类模板类的实例化类:
* WFHttpTask: protocol::HttpRequest, protocol::HttpResponse
* WFRedisTask: protocol::RedisRequest, protocol::RedisResponse
* WFMySQLTask: protocol::MySQLRequest, protocol::MySQLResponse
* WFDnsTask: protocol::DnsRequest, protocol::DnsResponse

## WFFileTask

此类模板类的实例化类(模板参数传入的是结构体):
* WFFileIOTask: FileIOArgs
* WFFileVIOTask: FileVIOArgs
* WFFileSyncTask: FileSyncArgs

## WFGenericTask

此类模板类的实例化类:
* WFEmptyTask
* WFDynamicTask

此模板的子类:
* WFGraphTask



## 未实例化的模板类

* WFThreadTask
* WFMultiThreadTask

## 无需实例化的类

* WFTimerTask
* WFCounterTask
* WFMailboxTask
* WFConditional

## 自身不需要，但是参数需要实例化的类

* WFGoTask: std::bind(std::forward<FUNC>(func), std::forward<ARGS>(args)...)


## 工厂类

### WFNetworkTaskFactory

> 需传入 class REQ, class RESP 两个参数。 

* create_client_task: TransportType，host，port，retry_max，callback
* create_server_task: CommService，std::function<void(T *)>


### WFThreadTaskFactory

> 需传入 class INPUT, class OUTPUT 两个参数。

类型:
* using T = WFThreadTask<INPUT, OUTPUT>;
* using MT = WFMultiThreadTask<INPUT, OUTPUT>;

工厂函数:
* create_thread_task: queue_name, std::function<void(INPUT *, OUTPUT *)>, callback
* create_multi_thread_task: queue_name, std::function<void(INPUT *, OUTPUT *)>, nthreads, callback
* create_thread_task: ExecQueue, Executor, std::function<void(INPUT *, OUTPUT *)>, callback


## inl 内部细节

### __WFGoTask

继承于 WFGoTask，相对于 WFGoTask, 多了一个 execute() 函数(实现了 ExecSession 类的execute)， 内部执行的是传入的 std::function<void()> &&func, 因此 WFGoTask 不需要传入 func， 但是可以 set_callback。

#### create_go_task

创建一个 __WFGoTask 对象， 传入 ExecQueue 对象(通过全局的 WFGlobal::get_exec_queue(queue_name) 创建), 传入 Executor 对象(WFGlobal::get_compute_executor() 创建), 传入 std::function, 由此可见，对于资源的申请，由统一的类分配。


### __WFDynamicTask

继承于 WFDynamicTask， 也就是继承于 WFGenericTask, 多了一个 dispatch 函数(实现了 WFGenericTask 的 dispatch 函数， 而 WFGenericTask 也是实现 SubTask 的此函数)，dispatch 的内部实现是将 传入的 std::function 推入**所在 series 的开头**，然后再执行父类的 dispatch。

#### create_dynamic_task

创建一个 __WFDynamicTask 对象， 传入一个名为 create 的 std::function 函数。

### WFComplexClientTask

> 继承于 WFClientTask, 需传入 class REQ, class RESP, typename CTX = bool 三个模板类参数。

比 WFClientTask 多了许多函数, 需传入 retry_max, std::function, 看上去就只增加了 retry_max 的处理。

protected functions(都留给子类去实现的):
* [virtual] init_success: 创建一个可用的连接
* [virtual] init_failed: 创建一个不可用的连接
* [virtual] check_request
* [virtual] route: 创建一个 WFRouterTask 任务，传入的参数是类本身的成员函数  route_callback。
* [virtual] finish_once

public functions:
* WFComplexClientTask: 构造函数，多了一个 retry_max。
* void init(const ParsedURI &uri): 调用 init_with_uri()
* void init(ParsedURI &&uri): 右值引用。
* init(TransportType type, const struct sockaddr *addr, socklen_t addrlen,
            const std::string &info): 根据底层 socket 来创建对象。
* set_transport_type： 设置传输类型, 包含以下几个。
    * TT_TCP
	* TT_UDP
	* TT_SCTP
	* TT_TCP_SSL
	* TT_SCTP_SSL
* get_transport_type： 获取传输类型
* [virtual] get_current_uri: 获取当前 uri
* set_redirect: 传入新的 uri, 在同样的连接上复用。
* set_redirect: 传入原生的 socket 参数，在同样的连接上复用。


protected functions:
* set_info: 设置 info 字符串。
* [virtual] dispatch: 任务的派遣和分发，派遣或结束的操作均依赖 state 的值。
* [virtual] done: 如果 router_task 未运行完，则手动从 series 中 pop 出去, 然后调用 finish_once。然后调用 switch_callback, 来切换task。
* clear_resp: 清理原来的 resp,通过调用对象的析构函数，然后在原来的空间上new一个新的对象，并设置他的 size_limit。
* disable_retry: 禁用 retry, 直接将已经 retry 的次数调至最大就可以了，而不是使用另一个标志位判断。

private functions:
* clear_prev_state: 清理遗留的状态，恢复成预设状态，为复用连接做准备。
* init_with_uri: 预设状态，然后调用 init_success 来初始化连接。 
* set_port: 根据一系列的判断，然后设置 uri_ 的 port ,
* router_callback： 分配给 create_router_task 使用的。
* switch_callback： 判断是否为复用连接，若是则调用 callback，若不是，则推入所在的 series。


protected members
* TransportType type_
* ParsedURI uri_;
* std::string info_;
* bool fixed_addr_;
* bool redirect_;
* CTX ctx_;
* int retry_max_;
* int retry_times_;
* WFNSPolicy *ns_policy_;
* WFRouterTask *router_task_;
* RouteManager::RouteResult route_result_;
* WFNSTracing tracing_;

### __WFThreadTask

继承于 WFGoTask，相对于 WFThreadTask, 多了一个 execute() 函数(实现了 ExecSession 类的execute)， 需要 ExecQueue,Executor,std::function<void(INPUT *, OUTPUT *)>,std::function<void(WFThreadTask<INPUT, OUTPUT> *)>四个参数。

### __WFThreadTask__

继承于 __WFThreadTask, 实现了 done 函数（起始于 SubTask）