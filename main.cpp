#include <iostream>
#include <string>

// Include your framework files here...
#include "MyGameMapper.hpp"
#include "RandomStrategy.hpp"
#include "GreedyStrategy.hpp"
#include "StrategyLoader.hpp"
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
    
    // ----------------------
    // Mode 1: Internal
    // ----------------------
    if (mode == "internal") {
        //std::cout << "[main] Internal mode is not fully implemented.\n";
        
        MyGameMapper game;
        game.read_cards("");  // Default card generation
        game.read_game("");    // Initialize table with 7s

        // register 4 players using the RandomStrategy
        for (uint64_t pid = 0; pid < 4; ++pid) {
            game.registerStrategy(pid, std::make_shared<sevens::RandomStrategy>());
        }

        // Play the game and display the results (rankings per player ID)
        auto results = game.compute_and_display_game(4);
        std::cout << "\nFinal results:\n";
        for (const auto& result : results) {
            std::cout << "Player " << result.first << " finished with rank " << result.second << "\n";
        }
    }

    // -----------------------
    // Mode 2: demo
    // -----------------------
    else if (mode == "demo") {
        //std::cout << "[main] Demo mode is not fully implemented.\n";
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

    // --------------------------
    // Mode 3: competition
    // --------------------------
    else if (mode == "competition") {
        if (argc < 3) {
            std::cout << "Usage: ./sevens_game competition <strategy1.dll> <strategy2.dll> ...\n";
            return 1;
        }

        MyGameMapper game;
        game.read_cards("");
        game.read_game("");

        std::vector<std::string> loaded_strategy_names;

        // Load each strategy passed via command line (starting from argv[2])
        for (int i = 2; i < argc; i++) {
            try {
                if (!StrategyLoader::isValidLibrary(argv[i])) {
                    std::cerr << "Invalid strategy library: " << argv[i] << "\n";
                    continue;
                }

                // attempt to load the strategy dynamically from the shared library
                auto strategy = StrategyLoader::loadFromLibrary(argv[i]);
                loaded_strategy_names.push_back(strategy->getName());
                // register it with the game
                game.registerStrategy(i-2, strategy);
                std::cout << "Successfully loaded strategy from " << argv[i] 
                         << " (" << strategy->getName() << ")\n";
            }
            catch (const std::exception& e) {
                std::cerr << "Error loading strategy from " << argv[i] << ":\n"
                         << e.what() << "\n";
                return 1;
            }
        }

        auto results = game.compute_and_display_game(argc - 2);
        
        std::cout << "\nFinal results:\n";
        std::cout << results.size() << std::endl;
        for (const auto& result : results) {
            std::cout << loaded_strategy_names[result.first] << " (Player " << result.first << ") finished with rank " 
                      << result.second << "\n";
        }
    }
    // ---------------------
    // Unknown mode
    // ---------------------
    else {
        std::cerr << "[main] Unknown mode: " << mode << std::endl;
    }
    
    return 0;
}
