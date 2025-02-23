#include "Engine/Game.h"

int main() {
    try {
        Game game;
        game.init();
        game.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
