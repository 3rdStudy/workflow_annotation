// 发起一个http请求

#include <iostream>
#include <workflow/Workflow.h>
#include <workflow/WFTaskFactory.h>
#include <workflow/WFFacilities.h>
#include <spdlog/spdlog.h>
#include <signal.h>

using namespace protocol;

#define REDIRECT_MAX 4
#define RETRY_MAX 2

void http_callback(WFHttpTask *task)
{
    HttpResponse *resp = task->get_resp();
    spdlog::info("Http status : {}", resp->get_status_code());

    // response body
    const void *body;
    size_t body_len;
    resp->get_parsed_body(&body, &body_len);

    // write body to file
    FILE *fp = fopen("res.txt", "w");
    fwrite(body, 1, body_len, fp);
    fclose(fp);

    spdlog::info("write file done");
}

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

int main()
{
    signal(SIGINT, sig_handler);
    std::string url = "http://www.baidu.com";
    // 通过create_xxx_task创建的对象为任务，一旦创建，必须被启动或取消
    WFHttpTask *task = WFTaskFactory::create_http_task(url,
                                                       REDIRECT_MAX,
                                                       RETRY_MAX,
                                                       http_callback);
    // 通过start,自行以task为first_task创建一个串行并理解启动任务
    // 任务start后，http_callback回调前，用户不能再操作该任务
    // 当http_callback任务结束后，任务立即被释放
    task->start();
    wait_group.wait();
}
