#pragma once

#include "Generic_game_mapper.hpp"
#include "PlayerStrategy.hpp"
#include <random>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace sevens {

/**
 * Enhanced Sevens simulation with strategy support:
 *  - Possibly internal mode or competition mode
 */
class MyGameMapper : public Generic_game_mapper {
private:
    std::unordered_map<uint64_t, Card> cards;
    std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>> tableLayout;
    std::unordered_map<uint64_t, std::vector<Card>> playerHands;
    std::unordered_map<uint64_t, std::shared_ptr<PlayerStrategy>> strategies;

    std::mt19937 rng;  // Random number generator
public:
    MyGameMapper();
    ~MyGameMapper() = default;

    std::vector<std::pair<uint64_t, uint64_t>>
    compute_game_progress(uint64_t numPlayers) override;

    std::vector<std::pair<uint64_t, uint64_t>>
    compute_and_display_game(uint64_t numPlayers) override;
    
    std::vector<std::pair<std::string, uint64_t>>
    compute_game_progress(const std::vector<std::string>& playerNames) override;
    
    std::vector<std::pair<std::string, uint64_t>>
    compute_and_display_game(const std::vector<std::string>& playerNames) override;

    // Required by Generic_card_parser and Generic_game_parser
    void read_cards(const std::string& filename) override;
    void read_game(const std::string& filename) override;
    
    // Strategy management
    void registerStrategy(uint64_t playerID, std::shared_ptr<PlayerStrategy> strategy);
    bool hasRegisteredStrategies() const;

    // Display table layout
    void print_table_layout() const;

private:
    // You can define any data structures needed to track the game
    // E.g., player hands, table layout, random engine, etc.
    // ...
};

} // namespace sevens
