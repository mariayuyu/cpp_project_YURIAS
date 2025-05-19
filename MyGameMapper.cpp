#include "MyGameMapper.hpp"
#include "MyCardParser.hpp"
#include "MyGameParser.hpp"
#include <iostream>
#include <stdexcept>

namespace sevens {

MyGameMapper::MyGameMapper() {
    rng.seed(std::random_device{}());
}

void MyGameMapper::print_table_layout() const {

    std::cout << "Current Table Layout:\n";
    for (uint64_t suit = 0; suit < 4; ++suit) {
        std::cout << "Suit " << suit << ": ";
        auto itSuit = table_layout.find(suit);
        for (uint64_t rank = 1; rank <= 13; ++rank) {
            if (itSuit != table_layout.end() && itSuit->second.count(rank) && itSuit->second.at(rank)) {
                std::cout << rank << " ";
            } else {
                std::cout << ". ";
            }
        }
        std::cout << "\n";
    }
}

void MyGameMapper::read_cards(const std::string& filename) {
    MyCardParser parser;
    parser.read_cards(filename);
    cards = parser.get_cards_hashmap();
    std::cout << "[MyGameMapper::read_cards] Loaded " << cards.size() << " cards.\n";
}

void MyGameMapper::read_game(const std::string& filename) {
    MyGameParser parser;
    parser.read_game(filename);
    table_layout = parser.get_table_layout();
    std::cout << "[MyGameMapper::read_game] Table layout initialized.\n";
}

bool MyGameMapper::hasRegisteredStrategies() const {
    return !strategies.empty();
}

void MyGameMapper::registerStrategy(uint64_t playerID, std::shared_ptr<PlayerStrategy> strategy) {
    (void)playerID;
    (void)strategy;
    strategies[playerID] = strategy;
    std::cout << "[MyGameMapper::registerStrategy] Registered strategy for player " << playerID << ".\n";
}

std::vector<std::pair<uint64_t, uint64_t>>
MyGameMapper::compute_game_progress(uint64_t numPlayers)
{
    std::cout << "[MyGameMapper::compute_game_progress] Simulating quietly...\n";

    // Distribute cards
    uint64_t i = 0;
    for (const auto& pair : cards) {
        const auto& card = pair.second;
        playerHands[i % numPlayers].push_back(card);
        ++i;
    }

    std::vector<std::pair<uint64_t, uint64_t>> rankings;
    std::vector<bool> finished(numPlayers, false);
    std::vector<uint64_t> playerRanks(numPlayers, 0);
    uint64_t rank = 1;
    bool changed;

    do {
        changed = false;
        for (uint64_t p = 0; p < numPlayers; ++p) {
            if (finished[p]) continue;
            auto& hand = playerHands[p];
            auto it = hand.begin();
            while (it != hand.end()) {
                const Card& card = *it;
                uint64_t s = card.suit;
                uint64_t r = card.rank;

                bool canPlay = (r == 7) ||
                               (r > 1 && table_layout[s][r - 1]) ||
                               (r < 13 && table_layout[s][r + 1]);

                if (canPlay) {
                    table_layout[s][r] = true;
                    it = hand.erase(it);
                    changed = true;
                    if (hand.empty()) {
                        finished[p] = true;
                        playerRanks[p] = rank++;
                    }
                    break;
                } else {
                    ++it;
                }
            }
        }
    } while (changed);

    for (uint64_t p = 0; p < numPlayers; ++p) {
        if (!finished[p]) playerRanks[p] = rank++;
        rankings.emplace_back(p, playerRanks[p]);
    }
    std::cout << "[MyGameMapper::compute_game_progress] Game progressing.\n";

    return rankings;
}

std::vector<std::pair<uint64_t, uint64_t>>
MyGameMapper::compute_and_display_game(uint64_t numPlayers)
{
    std::cout << "[MyGameMapper::compute_and_display_game] Starting simulation.\n";

    uint64_t i = 0;
    for (const auto& pair : cards) {
        const auto& card = pair.second;
        playerHands[i % numPlayers].push_back(card);
        ++i;
    }

    for (uint64_t p = 0; p < numPlayers; ++p) {
        std::cout << "Initial hand for Player " << p << ": ";
        for (const Card& c : playerHands[p]) {
            std::cout << c << " ";
        }
        std::cout << "\n";
    }

    std::vector<std::pair<uint64_t, uint64_t>> rankings;
    std::vector<bool> finished(numPlayers, false);
    std::vector<uint64_t> playerRanks(numPlayers, 0);
    uint64_t rank = 1;
    bool changed;

    do {
        changed = false;
        for (uint64_t p = 0; p < numPlayers; ++p) {
            if (finished[p]) continue;
            auto& hand = playerHands[p];
            bool played = false;
            auto it = hand.begin();
            while (it != hand.end()) {
                const Card& card = *it;
                uint64_t s = card.suit;
                uint64_t r = card.rank;

                bool canPlay = (r == 7) ||
                            (r > 1 && table_layout[s][r - 1]) ||
                            (r < 13 && table_layout[s][r + 1]);

                if (canPlay) {
                    std::cout << "Player " << p << " plays " << card << "\n";
                    table_layout[s][r] = true;
                    it = hand.erase(it);
                    played = true;
                    changed = true;
                    if (hand.empty()) {
                        finished[p] = true;
                        playerRanks[p] = rank++;
                        std::cout << "Player " << p << " finished with rank " << playerRanks[p] << "\n";
                    }
                    break;
                } else {
                    ++it;
                }
            }
            if (!played) {
                std::cout << "Player " << p << " cannot play this turn.\n";
            }
            print_table_layout();
        }
    } while (changed);

    for (uint64_t p = 0; p < numPlayers; ++p) {
        if (!finished[p]) playerRanks[p] = rank++;
        rankings.emplace_back(p, playerRanks[p]);
    }

    return rankings;
}

std::vector<std::pair<std::string, uint64_t>>
MyGameMapper::compute_game_progress(const std::vector<std::string>& playerNames)
{
    // Optional overload for name-based players
    std::cout << "[MyGameMapper::compute_game_progress(names)] TODO.\n";
    std::vector<std::pair<uint64_t, uint64_t>> idResults = compute_game_progress(playerNames.size());
    std::vector<std::pair<std::string, uint64_t>> results;
    for (const auto& pair : idResults) {
        results.emplace_back(playerNames[pair.first], pair.second);
    }
    return results;
}

std::vector<std::pair<std::string, uint64_t>>
MyGameMapper::compute_and_display_game(const std::vector<std::string>& playerNames)
{
    // Optional overload for name-based players
    (void)playerNames;
    std::cout << "[MyGameMapper::compute_and_display_game(names)] TODO.\n";
    std::vector<std::pair<uint64_t, uint64_t>> idResults = compute_and_display_game(playerNames.size());
    std::vector<std::pair<std::string, uint64_t>> results;

    for (const auto& pair : idResults) {
        results.emplace_back(playerNames[pair.first], pair.second);
    }
    return results;
}



} // namespace sevens