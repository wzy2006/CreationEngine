#include "network_c.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <cstring>
#include <iostream>
#include <thread>

using namespace std;
using namespace rapidjson;

TypeManager_c* tm_c;

void read_cb_c(struct bufferevent* bev, void* arg)
{
    char buf[1024] = {0};

    bufferevent_read(bev, buf, sizeof(buf));

    Document document;
    document.Parse(buf);
    if (document.HasParseError()) {
        return;
    }
    string type = document["type"].GetString();
    if (type == "registerBlock" && document.HasMember("registerList")) {
        const Value& list = document["registerList"];
        for (SizeType i = 0, l = list.Size(); i < l; i++) {
            if (list[i].HasMember("name") && list[i].HasMember("texturePath")) {
                tm_c->registerNode(
                    list[i]["name"].GetString(),
                    list[i]["texturePath"].GetString());
            }
        }
    }

    // bufferevent_write(bev, p, strlen(p) + 1);
}

void write_cb_c(struct bufferevent* bev, void* arg)
{
    ; // send to client properly
}

void event_cb_c(struct bufferevent* bev, short events, void* arg)
{
    if (events & BEV_EVENT_EOF) {
        cout << "客户端: 连接已关闭" << endl;
    } else if (events & BEV_EVENT_ERROR) {
        cout << "客户端: 未知网络错误" << endl;
    }
    if (events & BEV_EVENT_CONNECTED) {
        cout << "客户端: 成功连接至服务器" << endl;
    } else {
        bufferevent_free(bev);
        cout << "客户端: 连接已中断" << endl;
    }
}

void Network_c::init(TypeManager_c* tmPtr, int port)
{
    // configure
    tm_c = tmPtr;
    // 连接服务器
    base = event_base_new();
    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    evutil_inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr.s_addr);
    bufferevent_socket_connect(bev, (struct sockaddr*)&serv, sizeof(serv));

    // 设置回调
    bufferevent_setcb(bev, read_cb_c, write_cb_c, event_cb_c, NULL);
    bufferevent_enable(bev, EV_READ | EV_PERSIST);
}

void Network_c::eventloop()
{
    //启动循环监听
    thread netThread([=]() {
        event_base_dispatch(base);
        event_base_free(base);
    });
    netThread.detach();
}
void Network_c::startUp()
{
    Document output;
    Value type;
    output.SetObject();
    type = StringRef("register");
    output.AddMember("type", type, output.GetAllocator());
    StringBuffer sb;
    Writer<rapidjson::StringBuffer> write(sb);
    output.Accept(write);
    bufferevent_write(bev, sb.GetString(), sb.GetSize());
}
