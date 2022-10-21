# workflow-polaris
## 介绍

本项目是基于高性能的异步调度编程范式[C++ Workflow](https://github.com/sogou/workflow)以及腾讯开源的服务发现&服务治理平台[北极星](https://polarismesh.cn/#/)上构建的，目标是将workflow的服务治理能力和北极星治理能力相结合，提供更便捷、更丰富的服务场景。另外其他同型的服务治理系统也可以通过该项目实现与北极星平台的对接。

已支持功能：

* 基础功能：
  * 服务发现（包括根据集群要求自动定期更新服务信息）
  * 服务注册（包括健康检查：根据interval定期发心跳）
* 流量控制：
  * 路由管理（包括规则路由、元数据路由、就近路由）
  * 负载均衡

## 编译

```sh
git clone https://github.com/sogou/workflow-polaris.git
cd worflow-polaris
bazel build ...
```
## 运行

在example中有示例代码[cosumer_demo.cc](/example/consumer_demo.cc)和[provider_demo.cc](/example/provider_demo.cc)，编译成功后我们尝试一下运行consumer_demo:
```sh
./bazel-bin/example/consumer_demo
```
我们会得到demo的用法介绍：
```sh
USAGE:
    ./bazel-bin/example/consumer_demo <polaris_cluster> <namespace> <service_name> <query URL>

QUERY URL FORMAT:
    http://callee_service_namespace.callee_service_name:port#k1=v1&caller_service_namespace.caller_service_name

EXAMPLE:
    ./bazel-bin/example/consumer_demo http://127.0.0.1:8090 default workflow.polaris.service.b "http://default.workflow.polaris.service.b:8080#k1_env=v1_base&k2_number=v2_prime&a_namespace.a"
```

可以看到consumer_demo需要四个参数，分别是：
- polaris cluster：北极星集群地址
- namespace：使用北极星时的service namespace
- service_name：使用北极星时的service_name
- query URL：demo会帮我们尝试发一个请求，这个是用户请求的URL

我们按照提示的信息执行consumer_demo，配上我们的北极星服务信息：
```sh
./bazel-bin/example/consumer_demo http://polaris.cluster:8080 default workflow.polaris.service.b "http://default.workflow.polaris.service.b:8080#k1_env=v1_base&k2_number=v2_prime&a_namespace.a"
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

再看看provide_demo，参数里把集群地址、要上报的服务信息和地址端口填上即可:
```sh
USAGE:
    ./bazel-bin/example/provider_demo <polaris_cluster> <namespace> <service_name> <localhost> <port> [<service_token>]
```

如下是示例，然后屏幕上会有类似的结果：

```sh
./bazel-bin/example/provider_demo http://polaris.cluster:8080 default workflow.polaris.service.b 11.11.11.11 8002

Register Production workflow.polaris.service.b 11.11.11.11 8002 ret=0.
Success. Press "Enter" to deregister.

Deregister Production workflow.polaris.service.b 11.11.11.11 8002 ret=0.
```

## 使用步骤

#### 1. Consumer / Client / 主调方 / Caller

```cpp
// 1. 构造PolarisManager，传入集群地址，第二个参数可选传入yaml配置文件
PolarisManager mgr(polaris_url, /* polaris.config.yaml */);

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

#### 2. Provider / Server / 被调方 / Callee
```cpp
// 1. 构造PolarisManager
PolarisManager mgr(polaris_url);

// 2. 设置好instance的属性，并通过register接口把instance注册上去
//    其中有些服务注册时需要service_token，请参考北极星注册平台中的描述
//    heartbeat_interval是需要健康检查时的发送心跳间隔，如果不需要健康检查，此参数不生效
PolarisInstance instance;
instance.set_host(host);
instance.set_port(port);
int register_ret = mgr.register_service(service_namespace, service_name, service_token, heartbeat_interval, instance);
...

// 3. 需要退出之前可以调用deregister进行反注册
//    只有在register成功之后才可以调用deregister
bool deregister_ret = mgr.deregister_service(service_namespace, service_name, service_token, instance);
...

```

## 格式说明

被调方请求的拼接格式：

```sh
http://CALLEE_SERVICE_NAMESPACE.CALLEE_SERVICE_NAME:PORT#ROUTE_INFO&CALLER_SERVICE_NAMESPACE.CALLER_SERVICE_NAME
```

fragment里的ROUTE_INFO，是我们被调方的路由信息，路由信息有两种类型：
- 规则路由
- 元数据路由

这两种路由信息是不能同时启用的，只能启用其中一个，或者都不启用，由consumer/client/主调方在配置文件中指定，默认启用规则路由。

在规则路由中：`#k1=v1&k2=v2&caller_namespace.caller_name`，我们会得到<k1,v1>和<k2,v2>

而在元数据路由中：`#meta.k1=v1&meta.k2=v2`，我们会得到<k1,v1>和<k2,v2>

caller_namespace和caller_name对于规则路由有效，但对于元数据路由无效。

如果我们consumer/client/主调方在配置文件中配置了规则路由而非元数据路由，则`meta.k1=v1&meta.k2=v2`就会得到<meta.k1,v1>和<meta.k2,v2>。

对于主调方则简单很多，只需要通过接口把meta设置到用来注册的instance上即可被匹配：
```cpp
PolarisInstance instance;
std::map<std::string, std::string> meta = {{"k1", "v1"}};
instance.set_metadata(meta);
```

## 就近访问

如果需要就近访问，Consumer/Client/主调方需要把自己的地域信息，配到yaml配置文件中的如下三个域：

```yaml
global:
  api:
    location:
      region: north-china
      zone: beijing
      campus: wudaokou
```

对应的，如果Provider/Server/被调方希望自己的地域被注册到平台上，以方便client做匹配，可以把信息设置到instance上：

```cpp
PolarisInstance instance;
instance.set_region("north-china");
instance.set_zone("beijing");
instance.set_campus("wudaokou");
```

## 健康检查

被调方注册时，是可以开启健康检查功能的。

register、healthcheck、deregister的关系是：

1. 如果register的instance不打开healthcheck，那么注册成功后就是健康状态；
2. healthcheck目前只有一种实现方式，就是心跳，如果开启了，需要带上心跳的ttl，表示"没心跳多久之后被认为不健康"；
3. 如果打开了healthcheck，则按注册的时候填的heartbeat_interval参数定期发送心跳；
4. register的同步接口中包含了register网络请求，如果开启了healthcheck则还会包含第一次心跳请求。register接口返回错误的情况为：register失败、或者心跳请求拿到的错误码为不支持心跳（那么后续就不会定期发送心跳）；而如果第一次心跳仅为网络错误，后续依然会定期发送心跳。
5. deregister会在北极星平台彻底删掉这个instance，如果已开启healthcheck的话，会停止后台的定期心跳流程。

心跳示例代码大致如下：

```cpp
PolarisInstance instance;
instance.set_enable_healthcheck(true); // 默认不开启
instance.set_healthcheck_ttl(10); // 单位秒，默认5秒

ret = mgr.register_service(namespace, service_name, token, 2 /* 2秒发一次心跳 */, instance);
```
## 平台接入

部分私有化部署的北极星集群，除了平台服务可以观察到我们上报的instances以外，还有平台接入：用以拉取我们可以做服务发现/心跳上报等的北极星自己的服务器列表，service_name分别对应为polaris.discover和polaris.healthcheck。我们需要先从某个平台接入拉取这些列表，才能进行后续的上报/注册/发现等行为。

如果有平台接入，需要根据平台接入协议，填入Platform-Id和Platform-Token，让平台鉴权我们可以拉取服务器列表。因此这两个值可以填在PolarisManager的构造函数中：

```cpp
PolarisManager mgr(polaris_url, platform_id, platform_token, "polaris.yaml.template");
```
最后一个参数为北极星配置文件，不需要的话可以填空。

platform_id和platform_token：在Manager刚启动的时候、或者失败次数到达20次（默认），就会带上这两个值去平台接入重新拉取服务器。

