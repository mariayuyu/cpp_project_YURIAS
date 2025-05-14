#include "MyCardParser.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>

namespace sevens {

void MyCardParser::read_cards(const std::string& filename) {
    std::cout << "[MyCardParser::read_cards] Creating and shuffling 52-card deck.\n";

    // 首先创建有序的牌组
    std::vector<Card> deck;
    for (int suit = 0; suit < 4; ++suit) {
        for (int rank = 1; rank < 14; ++rank) {
            deck.push_back(Card{suit, rank});
        }
    }

    // 使用真随机数打乱牌组
    auto now = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()
    ).count();
    
    std::mt19937 rng(static_cast<unsigned long>(nanos));
    std::shuffle(deck.begin(), deck.end(), rng);

    // 将打乱后的牌放入hashmap
    int id = 0;
    for (const auto& card : deck) {
        this->cards_hashmap[id++] = card;
    }
}

} // namespace sevens