#include "server.hpp"

int main(int ac, char **av)
{
    if (ac != 2)
    {
        std::cout << "Error : must take an argument" << std::endl;
        return 1;
    }
    std::string filename = av[1];
    Parser object;
    object.parsing(av[1]);
    return 0;
}