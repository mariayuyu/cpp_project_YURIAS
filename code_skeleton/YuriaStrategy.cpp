// YuriaStrategy.cpp
#include "PlayerStrategy.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <random>
#include <chrono>
#include <iostream>

namespace sevens {

class YuriaStrategy : public PlayerStrategy {
public:
    YuriaStrategy() {
        auto seed = static_cast<unsigned long>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }

    ~YuriaStrategy() override = default;

    void initialize(uint64_t playerID) override {
        myID = playerID;
        round = 0;
        playedCards.clear();
        passCounts.clear();
        suitPlayability.clear();
        isEarlyGame = true;
        isMidGame = false;
        isLateGame = false;
        std::cout << "[Init] Player " << myID << " initialized.\n";
    }

    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override
    {
        round++;
        updateGamePhase();
        
        std::cout << "\n[Turn " << round << "] Player " << myID << " selecting card...\n";
        std::cout << "  Current hand: ";
        for (const auto& c : hand) std::cout << c << " ";
        std::cout << "\n";

        std::vector<std::pair<int, int>> candidates;
        std::unordered_map<int, int> suitCount;
        for (const auto& c : hand) suitCount[c.suit]++;
        
        // 更新每个花色的可玩性
        updateSuitPlayability(tableLayout);

        for (int i = 0; i < (int)hand.size(); ++i) {
            const Card& card = hand[i];
            if (!isPlayable(card, tableLayout)) continue;
            
            int score = evaluateCard(card, hand, tableLayout, suitCount);
            
            std::cout << "  -> Candidate " << card
                      << " | score=" << score 
                      << " | " << getEvaluationDetails(card, hand, tableLayout, suitCount) << "\n";

            candidates.emplace_back(i, score);
        }

        if (candidates.empty()) {
            std::cout << "  -> No playable cards. Passing.\n";
            return -1;
        }

        std::sort(candidates.begin(), candidates.end(),
                 [](auto& a, auto& b) { return a.second > b.second; });

        const Card& chosen = hand[candidates.front().first];
        std::cout << "  -> Playing: " << chosen << " (score=" << candidates.front().second << ")\n";
        return candidates.front().first;
    }

    void observeMove(uint64_t playerID, const Card& playedCard) override {
        playedCards[playedCard.suit].insert(playedCard.rank);
        passCounts[playerID] = 0;
        
        // 更新内部数据
        if (playedCard.rank == 7) {
            suitPlayability[playedCard.suit] = 2; // 高可玩性
        }
        
        std::cout << "[ObserveMove] Player " << playerID << " played " << playedCard << "\n";
    }

    void observePass(uint64_t playerID) override {
        passCounts[playerID]++;
        
        // 假设玩家连续pass两次，可能是特定花色被阻塞
        if (passCounts[playerID] >= 2) {
            blockProbabilities[playerID]++;
        }
        
        std::cout << "[ObservePass] Player " << playerID << " passed. Total passes: " << passCounts[playerID] << "\n";
    }

    std::string getName() const override {
        return "YuriaStrategy";
    }

private:
    uint64_t myID;
    int round = 0;
    std::mt19937 rng;
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> playedCards;
    std::unordered_map<uint64_t, int> passCounts;
    std::unordered_map<uint64_t, int> blockProbabilities; // 对手可能被阻塞的次数
    std::unordered_map<int, int> suitPlayability; // 花色可玩性：0=低，1=中，2=高
    
    // 游戏阶段标识
    bool isEarlyGame;
    bool isMidGame;
    bool isLateGame;

    void updateGamePhase() {
        int totalPlayed = 0;
        for (const auto& [suit, ranks] : playedCards) {
            totalPlayed += ranks.size();
        }
        
        if (totalPlayed < 10) {
            isEarlyGame = true;
            isMidGame = false;
            isLateGame = false;
        } else if (totalPlayed < 30) {
            isEarlyGame = false;
            isMidGame = true;
            isLateGame = false;
        } else {
            isEarlyGame = false;
            isMidGame = false;
            isLateGame = true;
        }
    }
    
    void updateSuitPlayability(const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) {
        for (int suit = 1; suit <= 4; suit++) {
            if (!tableLayout.count(suit)) {
                suitPlayability[suit] = 0; // 花色尚未开放
                continue;
            }
            
            const auto& layout = tableLayout.at(suit);
            int openEnds = 0;
            int coveredRanks = 0;
            
            for (int r = 1; r <= 13; r++) {
                if (layout.count(r) && layout.at(r)) {
                    coveredRanks++;
                    
                    // 检查是否是开放端
                    if ((r > 1 && (!layout.count(r-1) || !layout.at(r-1))) ||
                        (r < 13 && (!layout.count(r+1) || !layout.at(r+1)))) {
                        openEnds++;
                    }
                }
            }
            
            // 评估花色可玩性
            if (openEnds >= 3) suitPlayability[suit] = 2; // 高
            else if (openEnds >= 1) suitPlayability[suit] = 1; // 中
            else suitPlayability[suit] = 0; // 低
            
            // 如果该花色快要完成，提高其优先级
            if (coveredRanks >= 10) suitPlayability[suit] = 2;
        }
    }
    
    int evaluateCard(const Card& card, const std::vector<Card>& hand, 
                     const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
                     const std::unordered_map<int, int>& suitCount) {
        int score = 0;
        bool is7 = (card.rank == 7);
        bool isEdge = (card.rank == 1 || card.rank == 13);
        bool hasNeighbor = hasChain(card, hand);
        bool risky = opensBothEnds(card, tableLayout);
        int chainLength = calculateChainLength(card, hand);
        
        // 基础分数
        if (is7) score += 200;  // 7的基础高分
        
        // 根据游戏阶段调整策略
        if (isEarlyGame) {
            // 早期优先出7和创建连锁
            if (is7) score += 100;
            score += chainLength * 30;
            if (hasNeighbor) score += 70;
            if (isEdge) score += 20;
        } else if (isMidGame) {
            // 中期平衡牌链和清理花色
            score += chainLength * 50;
            if (suitCount.at(card.suit) >= 3) score += 60;
            if (isEdge) score += 40;
        } else { // 后期
            // 后期优先清理花色
            score += suitCount.at(card.suit) * 50;
            score += (13 - static_cast<int>(playedCards[card.suit].size())) * 10;
        }
        
        // 花色可玩性影响
        score += suitPlayability[card.suit] * 40;
        
        // 风险规避
        if (risky) score -= 80;
        
        // 对手分析
        int blockedPlayers = 0;
        for (const auto& [pid, count] : blockProbabilities) {
            if (count >= 2) blockedPlayers++;
        }
        if (blockedPlayers > 0) score += 30;
        
        // 特殊牌组合奖励
        if (isEdge) {
            if (card.rank == 1 && hasCardWithRank(hand, card.suit, 2)) score += 80;
            if (card.rank == 13 && hasCardWithRank(hand, card.suit, 12)) score += 80;
        }
        
        return score;
    }
    
    std::string getEvaluationDetails(const Card& card, const std::vector<Card>& hand, 
                                     const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
                                     const std::unordered_map<int, int>& suitCount) {
        std::string details;
        bool is7 = (card.rank == 7);
        bool isEdge = (card.rank == 1 || card.rank == 13);
        bool hasNeighbor = hasChain(card, hand);
        bool risky = opensBothEnds(card, tableLayout);
        int chainLength = calculateChainLength(card, hand);
        
        details += is7 ? "7-card " : "";
        details += isEdge ? "edge " : "";
        details += hasNeighbor ? "neighbor " : "";
        details += risky ? "risky " : "";
        details += "chain=" + std::to_string(chainLength) + " ";
        details += "sameSuit=" + std::to_string(suitCount.at(card.suit)) + " ";
        details += "playability=" + std::to_string(suitPlayability[card.suit]);
        
        return details;
    }

    bool isPlayable(const Card& card,
                     const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& table) {
        auto s = card.suit;
        auto r = card.rank;

        if (r == 7) {
            return !table.count(s) || !table.at(s).count(7) || !table.at(s).at(7);
        }

        if (!table.count(s)) return false;

        const auto& suitMap = table.at(s);
        return (r > 1 && suitMap.count(r - 1) && suitMap.at(r - 1)) ||
               (r < 13 && suitMap.count(r + 1) && suitMap.at(r + 1));
    }

    bool hasChain(const Card& card, const std::vector<Card>& hand) {
        for (const Card& c : hand) {
            if (c.suit == card.suit &&
                (c.rank == card.rank + 1 || c.rank == card.rank - 1)) {
                return true;
            }
        }
        return false;
    }

    bool opensBothEnds(const Card& card,
                        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& table) {
        auto s = card.suit;
        auto r = card.rank;
        if (!table.count(s)) return false;

        const auto& suitMap = table.at(s);
        return suitMap.count(r - 1) && suitMap.at(r - 1) &&
               suitMap.count(r + 1) && suitMap.at(r + 1);
    }
    
    int calculateChainLength(const Card& card, const std::vector<Card>& hand) {
        int length = 1; // 当前牌算一张
        int up = 1;
        int down = 1;
        
        // 向上计算连续牌
        while (hasCardWithRank(hand, card.suit, card.rank + up)) {
            length++;
            up++;
        }
        
        // 向下计算连续牌
        while (hasCardWithRank(hand, card.suit, card.rank - down)) {
            length++;
            down++;
        }
        
        return length;
    }
    
    bool hasCardWithRank(const std::vector<Card>& hand, uint64_t suit, uint64_t rank) {
        if (rank < 1 || rank > 13) return false;
        
        for (const Card& c : hand) {
            if (c.suit == suit && c.rank == rank) {
                return true;
            }
        }
        return false;
    }
};

extern "C" PlayerStrategy* createStrategy() {
    return new YuriaStrategy();
}

}  // namespace sevens
