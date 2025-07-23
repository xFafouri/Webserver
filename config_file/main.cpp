#include "parser.hpp"
#include <exception>

int main(int ac, char **av)
{
    if (ac > 2)
    {
        std::cout << "Error : must take an argument" << std::endl;
        return 1;
    }
    //must to hundle if there is no file of config file , i must to create a default one
    try{
        std::string filename;
        if (ac == 1)
            filename = "default.conf";
        else
            filename = av[1];
        Parser object;
        object.parsing(filename);

    }catch(std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}