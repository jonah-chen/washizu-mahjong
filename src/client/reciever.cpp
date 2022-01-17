#include "reciever.hpp"

R::R()
    : socket(context), server_endpoint(protocall::v4(), 10000)
{
    socket.connect(server_endpoint);

    t_recv = std::thread(&R::recv_impl, this);
    t_recv.detach();
}

R::message_type R::recv()
{
    message_type buf;
    std::unique_lock ul(local_m);
    local_cv.wait(ul, [this]{ return !q.empty(); });
    return q.pop_front();
}

void R::recv_impl()
{
    while(socket.is_open())
    {
        msg::buffer cur_msg;
        socket.receive(asio::buffer(cur_msg, msg::BUFFER_SIZE));
        if (msg::type(cur_msg) == msg::header::ping)
        {
            socket.send(asio::buffer(cur_msg, msg::BUFFER_SIZE));
        }
        else
        {
            q.push_back(std::move(cur_msg));
            local_cv.notify_one();
        }
    }
}
