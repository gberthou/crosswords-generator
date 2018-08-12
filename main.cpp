#include "ui.hpp"

const size_t WIDTH = 9;
const size_t HEIGHT = 11;

int main(void)
{
    UI ui(WIDTH, HEIGHT);
    ui.loop();
    return EXIT_SUCCESS;
}

