#include "RandomStrategy.hpp"
#include <algorithm>
#include <vector>
#include <chrono>
#include <random>
#include <iostream>

namespace sevens {

// Constructor seeds the RNG
RandomStrategy::RandomStrategy() {
    auto now = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()
    ).count();
    rng.seed(static_cast<unsigned long>(nanos));
}

void RandomStrategy::initialize(uint64_t playerID) {
    myID = playerID;
    auto now = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()
    ).count();
    rng.seed(static_cast<unsigned long>(nanos + playerID));
}

int RandomStrategy::selectCardToPlay(
    const std::vector<Card>& hand,
    const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout)
{
    if (hand.empty()) {
        return -1;
    }

    std::vector<int> validMoves;
    for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
        const Card& card = hand[i];
        uint64_t s = card.suit;
        uint64_t r = card.rank;

        bool canPlay = false;
        
        if (r == 7) {
            if (tableLayout.count(s) == 0 || 
                tableLayout.at(s).count(7) == 0 || 
                !tableLayout.at(s).at(7)) {
                canPlay = true;
            }
        }
        else if (r > 1 && tableLayout.count(s) > 0) {
            auto& suitLayout = tableLayout.at(s);
            if (suitLayout.count(r - 1) > 0 && suitLayout.at(r - 1) &&
                (suitLayout.count(r) == 0 || !suitLayout.at(r))) {
                canPlay = true;
            }
        }
        else if (r < 13 && tableLayout.count(s) > 0) {
            auto& suitLayout = tableLayout.at(s);
            if (suitLayout.count(r + 1) > 0 && suitLayout.at(r + 1) &&
                (suitLayout.count(r) == 0 || !suitLayout.at(r))) {
                canPlay = true;
            }
        }

        if (canPlay) {
            validMoves.push_back(i);
        }
    }

    if (validMoves.empty()) {
        return -1;
    }

    std::uniform_int_distribution<int> dist(0, static_cast<int>(validMoves.size()) - 1);
    int randomIndex = dist(rng);
    return validMoves[randomIndex];
}

void RandomStrategy::observeMove(uint64_t /*playerID*/, const Card& /*playedCard*/) {
}

void RandomStrategy::observePass(uint64_t /*playerID*/) {
}

std::string RandomStrategy::getName() const {
    return "RandomStrategy";
}


} // namespace sevens

#ifdef BUILD_SHARED_LIB
extern "C" sevens::PlayerStrategy* createStrategy() {
    return new sevens::RandomStrategy();
}
#endif