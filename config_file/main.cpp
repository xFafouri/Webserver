#include "parser.hpp"
#include <exception>

int main(int ac, char **av)
{
    if (ac != 2)
    {
        std::cout << "Error : must take an argument" << std::endl;
        return 1;
    }
    try{
        std::string filename = av[1];
        Parser object;
        object.parsing(av[1]);

    }catch(std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}