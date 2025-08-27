#include "server.hpp"


void Client::prepare_response() 
{
    std::cout << "response status = " << status << std::endl;
    if (is_cgi && status == 200)
    {
        std::cout << "CGI" << std::endl;
        run_cgi();
        response_ready = true;
    }
    else if (isGET && status == 200)
    {
        std::cout << "GET METHOD " << std::endl;
        getMethod();
        response_ready = true;
    }
    // else if (isDELETE)
    // {
    //     deleteMethod();
    // }
    else {
        response_buffer =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 13\r\n"
        "Connection: keep-alive\r\n"
        "\r\n"
        "Hello, world!";
        response_ready = true;
    }
}

bool Client::write_to_fd(int fd)
{
    std::cout << ">> Attempting to write to fd\n";
    std::cout << "response buffer size = " <<  response_buffer.size() << std::endl;
    std::cout << "send_offset = " << send_offset << std::endl;
    size_t remaining = response_buffer.size() - send_offset;
    std::cout << " remaining = " << std::endl;
    if (remaining == 0)
    {
        std::cout << "Done" << std::endl;
        return true;
    }

    ssize_t sent = send(fd, response_buffer.data() + send_offset, remaining, 0);
    std::cout << "send ====>" << sent << std::endl;

    if (sent > 0)
    {
        send_offset += sent;
        return (send_offset == response_buffer.size());
    }
    else if (sent < 0)
    {
        std::cout << "// wait for next EPOLLOUT" << std::endl;
        return false;
    }
    else
    {
        std::cout << "closed/error" << std::endl;
        return true;
    }
}

