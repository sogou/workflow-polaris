# workflow-polaris
## 介绍

本项目是基于高性能的异步调度编程范式[C++ Workflow](https://github.com/sogou/workflow)以及腾讯开源的服务发现&服务治理平台[北极星](https://polarismesh.cn/#/)上构建的，目标是将workflow的服务治理能力和北极星治理能力相结合，提供更便捷、更丰富的服务场景。另外其他同型的服务治理系统也可以通过该项目实现与北极星平台的对接。

功能：

- 基础功能：服务发现、服务注册
- 流量控制：负载均衡，路由管理

## 编译

```sh
git clone https://github.com/sogou/workflow-polaris.git
cd worflow-polaris
bazel build ...
```
## 运行

在example中有示例代码[demo.cc](/example/demo.cc)，编译成功后我们尝试一下运行：
```sh
./bazel-bin/example/demo
```
我们会得到demo的用法介绍：
```sh
USAGE:
    ./bazel-bin/example/demo <polaris cluster> <namespace> <service_name> <query URL>

QUERY URL FORMAT:
    http://callee_service_namespace.callee_service_name:port#k1=v1&caller_service_namespace.caller_service_name

EXAMPLE:
    ./bazel-bin/example/demo http://127.0.0.1:8090 default workflow.polaris.service.b "http://default.workflow.polaris.service.b:8080#k1_env=v1_base&k2_number=v2_prime&a_namespace.a"
```

可以看到demo需要四个参数，分别是：
- polaris cluster：北极星集群地址
- namespace：使用北极星时的service namespace
- service_name：使用北极星时的service_name
- query URL：demo会帮我们尝试发一个请求，这个是用户请求的URL

我们按照提示的信息执行demo，配上我们的北极星服务信息：
```sh
./bazel-bin/example/demo http://polaris.cluster:8090 default workflow.polaris.service.b "http://default.workflow.polaris.service.b:8080#k1_env=v1_base&k2_number=v2_prime&a_namespace.a"
```
屏幕上会打出以下结果：

```sh
Watch default workflow.polaris.service.b ret=0.
URL : http://default.workflow.polaris.service.b:8080#k1_env=v1_base&k2_number=v2_prime&a_namespace.a
Query task callback. state = 0 error = 0
Response from instance 127.0.0.0.1:8002
Unwatch default workflow.polaris.service.b ret=0.
Success. Press Ctrl-C to exit.
```

## 使用步骤

#### 1. Client / Consumer / 主调方 / Caller

```cpp
// 1. 构造PolarisManager
PolarisManager mgr(polaris_url);

// 2. 通过watch接口获取服务信息
int watch_ret = mgr.watch_service(service_namespace, service_name);

// 3. watch完毕就可以发送请求，workflow的本地命名服务会自动帮选取
WFHttpTask *task = WFTaskFactory::create_http_task(query_url,
                                                   3, /* REDIRECT_MAX */
                                                   5, /* RETRY_MAX */
                                                   [](WFHttpTask *task) {
    fprintf(stderr, "Query callback. state = %d error = %d\n",
            task->get_state(), task->get_error());
});

task->start();
...

// 4. 不使用的时候，可以调用unwatch，但不是必须的
//    只有在watch成功的service才可以正确调用unwatch
bool unwatch_ret = mgr.unwatch_service(service_namespace, service_name);
...

```

#### 2. Server/ Provider / 被调方 / Callee
```cpp
// 1. 构造PolarisManager
PolarisManager mgr(polaris_url);

// 2. 通过register接口把自己注册上去
int register_ret = mgr.register_service(service_namespace, service_name, instance);
...		

// 3. 需要退出之前可以调用deregister进行反注册
bool deregister_ret = mgr.unwatch_service(service_namespace, service_name);
...

```

## 格式说明

被调方请求的拼接格式：

```sh
http://callee_service_namespace.callee_service_name:port#route_info&caller_service_namespace.caller_service_name
```

fragment里的route_info，是我们被调方的路由信息，路由信息有两种类型：
- 规则路由
- 元数据路由

这两种路由信息是不能同时启用的，只能启用其中一个，或者都不启用，由client/consumer/主调方在配置文件中指定，默认启用规则路由。

在规则路由中：`#k1=v1&k2=v2&caller_namespace.caller_name`，我们会得到<k1,v1>和<k2,v2>

而在元数据路由中：`#meta.k1=v1&meta.k2=v2`，我们会得到<k1,v1>和<k2,v2>

caller_namespace和caller_name对于规则路由有效，但对于元数据路由无效。

如果我们client/consumer/主调方在配置文件中配置了规则路由而非元数据路由，则`meta.k1=v1&meta.k2=v2`就会得到<meta.k1,v1>和<meta.k2,v2>。
