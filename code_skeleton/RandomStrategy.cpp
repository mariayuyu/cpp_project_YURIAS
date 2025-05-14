#include "RandomStrategy.hpp"
#include <algorithm>
#include <vector>
#include <chrono>
#include <random>
#include <iostream>

namespace sevens {

// Constructor seeds the RNG
RandomStrategy::RandomStrategy() {
    // 使用当前时间的纳秒级时间戳作为种子
    auto now = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()
    ).count();
    rng.seed(static_cast<unsigned long>(nanos));
}

void RandomStrategy::initialize(uint64_t playerID) {
    myID = playerID;
    // 每次初始化时重新设置随机数种子
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

    // 找出所有可以合法出牌的选项
    std::vector<int> validMoves;
    for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
        const Card& card = hand[i];
        uint64_t s = card.suit;
        uint64_t r = card.rank;

        // 检查是否是合法的移动
        bool canPlay = false;
        
        // 检查是否是7
        if (r == 7) {
            // 检查这个花色的7是否已经被打出
            if (tableLayout.count(s) == 0 || 
                tableLayout.at(s).count(7) == 0 || 
                !tableLayout.at(s).at(7)) {
                canPlay = true;
            }
        }
        // 检查是否可以向下出牌（比如在8的情况下出7）
        else if (r > 1 && tableLayout.count(s) > 0) {
            auto& suitLayout = tableLayout.at(s);
            if (suitLayout.count(r - 1) > 0 && suitLayout.at(r - 1) &&
                (suitLayout.count(r) == 0 || !suitLayout.at(r))) {
                canPlay = true;
            }
        }
        // 检查是否可以向上出牌（比如在6的情况下出7）
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

    // 如果没有合法移动，返回-1表示跳过
    if (validMoves.empty()) {
        return -1;
    }

    // 从所有合法移动中随机选择一个
    std::uniform_int_distribution<int> dist(0, static_cast<int>(validMoves.size()) - 1);
    int randomIndex = dist(rng);
    return validMoves[randomIndex];
}

void RandomStrategy::observeMove(uint64_t /*playerID*/, const Card& /*playedCard*/) {
    // 这个简化版本不需要记录其他玩家的移动
}

void RandomStrategy::observePass(uint64_t /*playerID*/) {
    // 这个简化版本不需要记录其他玩家的跳过
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