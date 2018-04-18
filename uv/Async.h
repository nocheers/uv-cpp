/*
   Copyright 2017, object_he@yeah.net  All rights reserved.

   Author: object_he@yeah.net

   Last modified: 2018-4-18

   Description:
*/

#ifndef UV_ASYNC_H
#define UV_ASYNC_H

#include <memory>
#include <functional>

#include "EventLoop.h"



namespace uv
{


template<typename ValueType>
class Async  : public std::enable_shared_from_this<Async<ValueType>>
{
public:
    using AsyncCallback = std::function<void(Async<ValueType>*, ValueType)>;


    Async<ValueType>(EventLoop* loop,AsyncCallback callback, ValueType value)
        : handle_(new uv_async_t),
        callback_(callback),
        value_(value)
    {
        ::uv_async_init(loop->hanlde(), handle_, Async<ValueType>::process);
        handle_->data = static_cast<void*>(this);
    }

    virtual ~Async<ValueType>()
    {
        if (uv_is_closing((uv_handle_t*)handle_) == 0)
        {
            ::uv_close((uv_handle_t*)handle_, [](uv_handle_t* handle)
            {
                delete (uv_async_t*)handle;
            });
        }
    }

    void setData(ValueType value)
    {
        value_ = value;
    }

    void runInLoop()
    {
        ::uv_async_send(handle_);
    }

private:
    uv_async_t* handle_;
    AsyncCallback callback_;
    ValueType value_;

    static void process(uv_async_t* handle)
    {
        auto async = static_cast<Async*>(handle->data);
        if(async->callback_)
            async->callback_(async,async->value_);
    }
};


template<typename ValueType>
using AsyncPtr = std::shared_ptr<Async<ValueType>>;
}
#endif
