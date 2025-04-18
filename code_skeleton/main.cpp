#include <iostream>
#include <string>

// Include your framework files here...
#include "MyGameMapper.hpp"
#include "RandomStrategy.hpp"
#include "GreedyStrategy.hpp"
// #include "StrategyLoader.hpp"
using namespace sevens;


int main(int argc, char* argv[]) {
    // This is a minimal skeleton for demonstration purposes.
    // Students should integrate their classes or call the relevant
    // game logic from MyGameMapper (or other classes) as needed.
    
    if (argc < 2) {
        std::cout << "Usage: ./sevens_game [mode] [optional libs...]\n";
        return 1;
    }
    
    std::string mode = argv[1];
    
    if (mode == "internal") {
        std::cout << "[main] Internal mode is not fully implemented.\n";
        
        MyGameMapper game;
        game.read_cards("");  // Default card generation
        game.read_game("");    // Initialize table with 7s

        for (uint64_t pid = 0; pid < 4; ++pid) {
            game.registerStrategy(pid, std::make_shared<sevens::RandomStrategy>());
        }

        auto results = game.compute_and_display_game(4);
        std::cout << "\nFinal results:\n";
        for (const auto& result : results) {
            std::cout << "Player " << result.first << " finished with rank " << result.second << "\n";
        }
    }
    else if (mode == "demo") {
        std::cout << "[main] Demo mode is not fully implemented.\n";
        std::vector<std::string> playerNames = {"Alice", "Bob", "Charlie", "Dana"};

        MyGameMapper game;
        game.read_cards("");
        game.read_game("");

        for (size_t i = 0; i < playerNames.size(); ++i) {
            game.registerStrategy(i, std::make_shared<sevens::RandomStrategy>());
        }

        auto results = game.compute_and_display_game(playerNames);
        std::cout << "\nFinal results:\n";
        for (const auto& result : results) {
            std::cout << result.first << " finished with rank " << result.second << "\n";
        }
    }
    else if (mode == "competition") {
        std::cout << "[main] Competition mode is not fully implemented.\n";
        // TODO: Dynamically load .so libraries from argv[2..] 
        // and simulate a competition
    }
    else {
        std::cerr << "[main] Unknown mode: " << mode << std::endl;
    }
    
    return 0;
}
