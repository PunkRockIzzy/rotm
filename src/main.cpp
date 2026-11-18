#include "core.hpp"

int main(int argc, char *argv[])
{
    rotm::Core core = rotm::Core{argc, argv};

    core.run();
}
