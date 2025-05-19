#include "MyCardParser.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>

namespace sevens {

void MyCardParser::read_cards(const std::string& filename) {
    std::cout << "[MyCardParser::read_cards] Creating and shuffling 52-card deck.\n";

    std::vector<Card> deck;
    for (int suit = 0; suit < 4; ++suit) {
        for (int rank = 1; rank < 14; ++rank) {
            deck.push_back(Card{suit, rank});
        }
    }

    auto now = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()
    ).count();
    
    std::mt19937 rng(static_cast<unsigned long>(nanos));
    std::shuffle(deck.begin(), deck.end(), rng);

    int id = 0;
    for (const auto& card : deck) {
        this->cards_hashmap[id++] = card;
    }
}

} // namespace sevens