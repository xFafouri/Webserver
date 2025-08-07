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

    if (!response_ready || response_buffer.empty()) 
    {
        std::cout << ">> Not ready to write yet\n";
        return true; // Nothing to write yet
    }

    ssize_t sent = send(fd, response_buffer.c_str(), response_buffer.length(), 0);
    std::cout << ">> Sent bytes: " << sent << "\n";

    if (sent <= 0)
    {
        std::cerr << ">> Failed to send response or client closed connection\n";
        return false;
    }

    if (static_cast<size_t>(sent) == response_buffer.length()) {
        std::cout << ">> Response sent completely\n";
        response_buffer.clear();
        response_ready = false;
        close(client_fd);
        return true;
    }

    response_buffer = response_buffer.substr(sent);
    std::cout << ">> Partial send. Remaining: " << response_buffer.length() << "\n";
    return false;
}
