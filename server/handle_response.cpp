#include "server.hpp"
#include <sys/types.h>

void Client::prepare_response() 
{
    // std::cout << "response status = " << status << std::endl;
    if (isGET && status == 200 && cgi_error_code == 0)
    {
        getMethod();
        response_ready = true;
    }
    else if (isDELETE)
    {
        deleteMethod();
    }
    else if (isPOST && status == 200)
    {
        // std::cout << "is POST here !!" << std::endl;
        response_buffer =
        "HTTP/1.1 201 created OK\r\n"
        "Content-Length: 10\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Post Done!";
        response_ready = true;
    }
    else 
    {
        sendError(status);
        response_ready = true;
    }
}

bool Client::write_to_fd(int fd)
{
    // std::cout << ">> Attempting to write to fd\n";
    // std::cout << "response buffer size = " <<  response_buffer.size() << std::endl;
    // std::cout << "response buffer  = " <<  response_buffer << std::endl;
    // std::cout << "send_offset = " << send_offset << std::endl;
    size_t remaining = response_buffer.size() - send_offset;
    // std::cout << "remaining = " << remaining << std::endl;
    if (remaining == 0)
    {
        return true;
    }

    ssize_t sent = send(fd, response_buffer.data() + send_offset, remaining, 0);
    // std::cout << "send ====>" << sent << std::endl;
    
    if (sent > 0)
    {
        // is_sending_response = true;
        send_offset += sent;
        // std::cout << "send_offset = " << send_offset << std::endl;
        last_activity = now_ms();
        return (send_offset == (ssize_t)response_buffer.size());
    }
    else if (sent < 0)
    {
        // std::cout << "// wait for next EPOLLOUT" << std::endl;
        return false;
    }
    else
    {
        // std::cout << "closed/error" << std::endl;
        return true;
    }
}
