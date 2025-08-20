#include "server.hpp"


void Client::prepare_response() 
{
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

// bool Client::write_to_fd(int fd)
// {
//     // if (!response_ready || response_buffer.empty())
//     //     return true;

    // std::cout << ">> Attempting to write to fd\n";
//     remaining = 0;
//     send_len = response_buffer.size() - send_offset;
//     std::cout << "response buffer size = " <<  response_buffer.size() << std::endl;
//     remaining = response_buffer.size()  - send_offset;
//     std::cout << "remaining = " << remaining << std::endl;
//     if (remaining == 0) 
//     {
//         // Done sending, close if needed
//         // close(client_fd);
//         return true;
//     }
//     ssize_t chunk_size = 8192;

//     // ssize_t to_send = std::min(chunk_size, remaining);
    
//     ssize_t sent = send(client_fd, response_buffer.data() + send_offset, send_len - sent, 0);
//     std::cout << "send ====>" << sent << std::endl;
//     if (sent > 0) 
//     {
//         send_offset += sent;
//         if (send_offset >= response_buffer.size()) 
//         {
//             return true;
//         }
//         return false;
//     }
//     else if (sent < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) 
//     {
//         return false;
//     }
//     else 
//     {
//         return true;
//     }
//     if (static_cast<size_t>(sent) == response_buffer.length()) {
//         std::cout << ">> Response sent completely\n";
//         response_buffer.clear();
//         response_ready = false;
//         close(client_fd);
//         return true;
//     }

//     response_buffer = response_buffer.substr(sent);
//     std::cout << ">> Partial send. Remaining: " << response_buffer.length() << "\n";
//     return false;
// }
