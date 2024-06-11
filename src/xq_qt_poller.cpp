/***************************************************************************
* Copyright (c) 2021, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include <iostream>

#include "xq/xq_qt_poller.hpp"

namespace xeus
{

    WorkerThread::WorkerThread(QObject* parent,
                               poller_t poller)
        : QThread(parent)
        , m_poller(std::move(poller))
        , m_request_stop(false)
    {
        std::cout << "starting thread" << std::endl;
    }

    void WorkerThread::run()
    {
        while(!m_request_stop.load())
        {
            auto msg = m_poller(10);
            if (msg)
            {
                if (msg.value().second == channel::SHELL)
                {
                    // signals do not like the move semantics so
                    // we need to put in a pointer and delete it on the receiving end
                    xeus::xmessage* pmsg = new xeus::xmessage(std::move(msg.value().first));
                    emit received_shell_msg_signal(pmsg);
                }
                else
                {
                    // signals do not like the move semantics so
                    // we need to put in a pointer and delete it on the receiving end
                    xeus::xmessage* pmsg = new xeus::xmessage(std::move(msg.value().first));
                    emit received_controll_msg_signal(pmsg);
                }
            }
        }
    }

    void WorkerThread::stop()
    {
        m_request_stop = true;
    }
}

/*WorkerThread::WorkerThread(QObject* parent,
                           zmq::socket_t* p_shell,
                           zmq::socket_t* p_controller,
                           xeus::xauthentication* auth)
    :
    QThread(parent),
    p_shell(p_shell),
    p_controller(p_controller),
    m_request_stop(false)
{
    std::cout << "starting thread" << std::endl;
    p_auth = auth;
}

void WorkerThread::stop()
{
    m_request_stop = true;
}

void WorkerThread::run()
{
    zmq::pollitem_t items[]
        = { { *p_controller, 0, ZMQ_POLLIN, 0 }, { *p_shell, 0, ZMQ_POLLIN, 0 } };

    while(!m_request_stop.load())
    {
        zmq::poll(&items[0], 2, std::chrono::milliseconds(10));
        {
            try
            {
                if (items[0].revents & ZMQ_POLLIN)
                {
                    zmq::multipart_t  wire_msg;
                    std::cout << "msg inside poll" << std::endl;
                    wire_msg.recv(*p_controller);
                    xeus::xmessage msg = xeus::xzmq_serializer::deserialize(wire_msg, *p_auth);

                    // signals do not like the move semantics so
                    // we need to put in a pointer and delete it on the receiving end
                    xeus::xmessage* pmsg = new xeus::xmessage(std::move(msg));
                    emit received_controll_msg_signal(pmsg);
                }

                if (!m_request_stop.load() && (items[1].revents & ZMQ_POLLIN))
                {
                    zmq::multipart_t  wire_msg;
                    wire_msg.recv(*p_shell);
                    xeus::xmessage msg = xeus::xzmq_serializer::deserialize(wire_msg, *p_auth);

                    // signals do not like the move semantics so 
                    // we need to put in a pointer and delete it on the receiving end
                    xeus::xmessage * pmsg = new xeus::xmessage(std::move(msg));
                    emit received_shell_msg_signal(pmsg);
                }
            }
            catch (std::exception& e)
            {
                std::cerr << e.what() << std::endl;
            }
        }
    }
}*/
