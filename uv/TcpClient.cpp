/*
   Copyright 2017, object_he@yeah.net  All rights reserved.

   Author: object_he@yeah.net

   Last modified: 2018-4-18

   Description:
*/

#include <string>

#include "TcpClient.h"
#include "LogInterface.h"

using namespace uv;
using namespace std;


TcpClient::TcpClient(EventLoop* loop)
    :loop_(loop),
    socket_(new uv_tcp_t()),
    connect_(new uv_connect_t()),
    ipv(SocketAddr::Ipv4),
    connectCallback_(nullptr),
    onMessageCallback_(nullptr),
    onConnectCloseCallback_(nullptr),
    connection_(nullptr)
{
    ::uv_tcp_init(loop->hanlde(), socket_);
    socket_->data = static_cast<void*>(this);
}

TcpClient::~TcpClient()
{
    delete connect_;
}


void TcpClient::connect(SocketAddr& addr)
{
    ipv = addr.Ipv();
    ::uv_tcp_connect(connect_, socket_, addr.Addr(), [](uv_connect_t* req, int status)
    {
        auto handle = static_cast<TcpClient*>(((uv_tcp_t *)(req->handle))->data);
        if (0 != status)
        {
            uv::Log::Instance()->error( "connect fail.");
            handle->onConnect(false);
            return;
        }

        handle->onConnect(true);

    });
}

void TcpClient::onConnect(bool successed)
{
    if(successed)
    {
        string name;
        SocketAddr::AddrToStr(socket_,name,ipv);

        connection_ = make_shared<TcpConnection>(loop_, name, socket_);
        connection_->setMessageCallback(std::bind(&TcpClient::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
        connection_->setConnectCloseCallback(std::bind(&TcpClient::onConnectClose,this,std::placeholders::_1));
    }
    else
    {
        updata();
    }
    if(connectCallback_)
        connectCallback_(successed);
}
void TcpClient::onConnectClose(string& name)
{
    if (connection_)
    {
        connection_->close([this](std::string& name)
        {
            //this old socket_ point will release in connection object release when re-connect.
            socket_ = new uv_tcp_t();
            updata();
            uv::Log::Instance()->info("Close tcp client connection complete.");
            if (onConnectCloseCallback_)
                onConnectCloseCallback_();
        });
    }

}
void TcpClient::onMessage(shared_ptr<TcpConnection> connection,const char* buf,ssize_t size)
{
    if(onMessageCallback_)
        onMessageCallback_(buf,size);
}

void uv::TcpClient::close(std::function<void(std::string&)> callback)
{
    if (connection_)
    {
        connection_->close(callback);
    }
    else
    {
        std::string str("");
        callback(str);
    }
}

void uv::TcpClient::write(const char* buf, unsigned int size, AfterWriteCallback callback)
{
    if (connection_)
    {
        connection_->write(buf, size, callback);
    }
    else
    {
        WriteInfo info = { WriteInfo::Disconnected,const_cast<char*>(buf),size };
        callback(info);
    }

}

void uv::TcpClient::writeInLoop(const char * buf, unsigned int size, AfterWriteCallback callback)
{
    if (connection_)
    {
        connection_->writeInLoop(buf, size, callback);
    }
    else
    {
        WriteInfo info = { WriteInfo::Disconnected,const_cast<char*>(buf),size };
        callback(info);
    }
}

void uv::TcpClient::setConnectCallback(ConnectCallback callback)
{
    connectCallback_ = callback;
}

void uv::TcpClient::setMessageCallback(NewMessageCallback callback)
{
    onMessageCallback_ = callback;
}

void uv::TcpClient::setConnectCloseCallback(OnConnectClose callback)
{
    onConnectCloseCallback_ = callback;
}

EventLoop* uv::TcpClient::Loop()
{
    return loop_;
}

void TcpClient::updata()
{
    ::uv_tcp_init(loop_->hanlde(), socket_);
    socket_->data = static_cast<void*>(this);
}
