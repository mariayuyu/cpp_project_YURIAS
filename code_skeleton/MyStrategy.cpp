// MyStrategy.cpp
#include "PlayerStrategy.hpp"
#include <algorithm>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <cmath>

namespace sevens {

class SmartStrategy : public PlayerStrategy {
public:
    SmartStrategy() {
        auto seed = static_cast<unsigned long>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }
    
    ~SmartStrategy() override = default;
    
    void initialize(uint64_t playerID) override {
        myID = playerID;
        // 清空所有历史记录
        playedCards.clear();
        playerPasses.clear();
        playerHistory.clear();
        suitPreference.clear();
        playerSuitMissing.clear();
        moveCount = 0;
    }
    
    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        if (hand.empty()) {
            return -1;
        }

        moveCount++; // 记录回合数
        
        // 分析游戏阶段
        GameStage stage = analyzeGameStage(hand, tableLayout);

        // 找出所有合法的移动
        std::vector<std::pair<int, int>> validMoves; // <index, score>
        for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
            const Card& card = hand[i];
            if (isValidMove(card, tableLayout)) {
                // 根据不同阶段计算分数
                int score = calculateMoveScore(hand, card, tableLayout, stage);
                validMoves.emplace_back(i, score);
            }
        }

        if (validMoves.empty()) {
            return -1;
        }

        // 动态调整随机性
        double randomThreshold = getRandomThreshold(stage, hand.size());
        
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng) < randomThreshold) {
            // 选择得分最高的移动
            auto bestMove = std::max_element(
                validMoves.begin(), 
                validMoves.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; }
            );
            return bestMove->first;
        } else {
            // 随机选择一个较好的移动（前50%的移动中随机选择）
            std::sort(validMoves.begin(), validMoves.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            int topMoves = std::max(1, static_cast<int>(validMoves.size() / 2));
            std::uniform_int_distribution<int> moveDist(0, topMoves - 1);
            return validMoves[moveDist(rng)].first;
        }
    }
    
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        // 记录已经打出的牌
        playedCards[playedCard.suit].insert(playedCard.rank);
        
        // 更新玩家历史
        playerHistory[playerID].push_back(playedCard);
        suitPreference[playedCard.suit]++;
        
        // 清除该玩家缺少这个花色的标记
        if (playerSuitMissing.count(playerID) > 0) {
            playerSuitMissing[playerID].erase(playedCard.suit);
        }
        
        // 重置该玩家的连续跳过次数
        playerPasses[playerID] = 0;
        
        // 分析玩家偏好
        updatePlayerPreferences(playerID, playedCard);
    }
    
    void observePass(uint64_t playerID) override {
        playerPasses[playerID]++;
        
        // 如果连续跳过多次，可能缺少某些花色
        if (playerPasses[playerID] >= 2) {
            analyzePlayerMissingSuits(playerID);
        }
    }
    
    std::string getName() const override {
        return "SmartStrategy";
    }

private:
    uint64_t myID;
    std::mt19937 rng;
    int moveCount;  // 记录游戏进行的回合数
    
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> playedCards;
    std::unordered_map<uint64_t, int> playerPasses;
    std::unordered_map<uint64_t, std::vector<Card>> playerHistory;
    std::unordered_map<uint64_t, int> suitPreference;
    
    // 新增：记录玩家可能缺少的花色
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> playerSuitMissing;
    
    // 新增：记录玩家的出牌偏好
    std::unordered_map<uint64_t, std::unordered_map<uint64_t, int>> playerSuitPreferences;

    enum class GameStage {
        EARLY,    // 开局阶段
        MIDDLE,   // 中局阶段
        LATE      // 残局阶段
    };

    // 分析游戏阶段
    GameStage analyzeGameStage(const std::vector<Card>& hand,
                              const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) {
        // 根据已出牌数量和手牌数量判断游戏阶段
        int totalPlayed = 0;
        for (const auto& [suit, cards] : playedCards) {
            totalPlayed += cards.size();
        }
        
        if (totalPlayed < 12) return GameStage::EARLY;
        if (hand.size() <= 4) return GameStage::LATE;
        return GameStage::MIDDLE;
    }

    // 判断是否是关键牌
    bool isKeyCard(const Card& card, const std::vector<Card>& hand) {
        // 检查是否有连续的牌
        bool hasLower = false, hasHigher = false;
        for (const Card& c : hand) {
            if (c.suit == card.suit) {
                if (c.rank == card.rank - 1) hasLower = true;
                if (c.rank == card.rank + 1) hasHigher = true;
            }
        }
        return hasLower && hasHigher;
    }

    // 分析玩家缺少的花色
    void analyzePlayerMissingSuits(uint64_t playerID) {
        std::unordered_set<uint64_t>& missingSuits = playerSuitMissing[playerID];
        
        // 检查每个花色
        for (uint64_t suit = 0; suit < 4; suit++) {
            // 如果玩家长时间没有出某个花色的牌，而该花色有可出的牌
            if (playerHistory[playerID].empty() || 
                std::none_of(playerHistory[playerID].begin(), 
                           playerHistory[playerID].end(),
                           [suit](const Card& c) { return c.suit == suit; })) {
                missingSuits.insert(suit);
            }
        }
    }

    // 更新玩家偏好
    void updatePlayerPreferences(uint64_t playerID, const Card& card) {
        playerSuitPreferences[playerID][card.suit]++;
    }

    // 获取随机阈值
    double getRandomThreshold(GameStage stage, int handSize) {
        switch (stage) {
            case GameStage::EARLY:
                return 0.7;  // 开局更多尝试
            case GameStage::LATE:
                return 0.95; // 残局更确定性
            default:
                return handSize <= 3 ? 0.9 : 0.8;
        }
    }

    // 检查移动是否合法
    bool isValidMove(const Card& card, 
                    const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) {
        uint64_t s = card.suit;
        uint64_t r = card.rank;

        // 检查是否是7
        if (r == 7) {
            return tableLayout.count(s) == 0 || 
                   tableLayout.at(s).count(7) == 0 || 
                   !tableLayout.at(s).at(7);
        }
        
        // 检查其他情况
        if (tableLayout.count(s) > 0) {
            auto& suitLayout = tableLayout.at(s);
            // 向下出牌
            if (r > 1 && suitLayout.count(r - 1) > 0 && suitLayout.at(r - 1) &&
                (suitLayout.count(r) == 0 || !suitLayout.at(r))) {
                return true;
            }
            // 向上出牌
            if (r < 13 && suitLayout.count(r + 1) > 0 && suitLayout.at(r + 1) &&
                (suitLayout.count(r) == 0 || !suitLayout.at(r))) {
                return true;
            }
        }
        return false;
    }

    // 改进的评分计算函数
    int calculateMoveScore(const std::vector<Card>& hand, const Card& card,
                          const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
                          GameStage stage) 
    {
        int score = 0;
        uint64_t s = card.suit;
        uint64_t r = card.rank;

        // 1. 基础分（根据游戏阶段调整）
        switch (stage) {
            case GameStage::EARLY:
                // 开局策略：优先出7，但要考虑是否会给对手创造机会
                if (r == 7) {
                    // 如果这个花色还没有牌，优先开启新花色
                    if (tableLayout.count(s) == 0) {
                        score += 250;  // 大幅提高开新花色的权重
                    } else {
                        score += 150;
                    }
                }
                else if (r == 6 || r == 8) score += 120;
                else score += 80;
                break;
                
            case GameStage::MIDDLE:
                // 中局策略：控制节奏，避免给对手简单机会
                if (isKeyCard(card, hand)) {
                    // 如果这张牌出了会给对手创造连续出牌的机会，降低其优先级
                    if (willCreateEasyPlay(card, tableLayout)) {
                        score += 50;
                    } else {
                        score += 150;  // 否则优先出这张牌
                    }
                } else {
                    score += 100;
                }
                break;
                
            case GameStage::LATE:
                // 残局策略：快速出牌，但仍要考虑不给对手太多机会
                score += 180;
                break;
        }

        // 2. 评估出牌后的局势
        int situationScore = evaluatePostPlaySituation(card, hand, tableLayout);
        score += situationScore;

        // 3. 花色控制评估
        int suitControlScore = evaluateSuitControl(card, hand, tableLayout);
        score += suitControlScore;

        return score;
    }

    // 新增：评估出牌是否会给对手创造简单机会
    bool willCreateEasyPlay(const Card& card, 
                           const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) 
    {
        uint64_t s = card.suit;
        uint64_t r = card.rank;
        
        // 检查出牌后是否会创造容易的出牌机会
        int easyPlays = 0;
        
        // 检查上下相邻的牌
        if (r > 1) {  // 检查较小的牌
            if (tableLayout.count(s) == 0 || 
                tableLayout.at(s).count(r-1) == 0) {
                easyPlays++;
            }
        }
        if (r < 13) {  // 检查较大的牌
            if (tableLayout.count(s) == 0 || 
                tableLayout.at(s).count(r+1) == 0) {
                easyPlays++;
            }
        }
        
        return easyPlays > 1;  // 如果会创造超过一个容易的出牌机会，返回true
    }

    // 新增：评估出牌后的局势
    int evaluatePostPlaySituation(const Card& card, 
                                const std::vector<Card>& hand,
                                const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) 
    {
        int score = 0;
        uint64_t s = card.suit;
        uint64_t r = card.rank;

        // 1. 评估是否会打开太多出牌机会
        int openEnds = 0;
        if (r > 1 && tableLayout.count(s) > 0 && tableLayout.at(s).count(r-1) > 0) openEnds++;
        if (r < 13 && tableLayout.count(s) > 0 && tableLayout.at(s).count(r+1) > 0) openEnds++;
        
        if (openEnds > 1) {
            score -= 40;  // 降低会打开多个出牌机会的牌的分数
        }

        // 2. 评估是否能形成有利的连续出牌机会
        bool canPlayContinuously = false;
        for (const Card& c : hand) {
            if (c.suit == s && (c.rank == r-1 || c.rank == r+1)) {
                canPlayContinuously = true;
                break;
            }
        }
        
        if (canPlayContinuously) {
            score += 30;  // 如果能形成连续出牌机会，适当加分
        }

        return score;
    }

    // 新增：评估花色控制
    int evaluateSuitControl(const Card& card,
                           const std::vector<Card>& hand,
                           const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) 
    {
        int score = 0;
        uint64_t s = card.suit;
        
        // 1. 统计手中该花色的牌数
        int suitCount = 0;
        std::vector<uint64_t> suitRanks;
        for (const Card& c : hand) {
            if (c.suit == s) {
                suitCount++;
                suitRanks.push_back(c.rank);
            }
        }

        // 2. 评估花色控制能力
        if (suitCount >= 3) {
            // 检查是否有连续的牌
            std::sort(suitRanks.begin(), suitRanks.end());
            int consecutiveCount = 1;
            int maxConsecutive = 1;
            
            for (size_t i = 1; i < suitRanks.size(); i++) {
                if (suitRanks[i] == suitRanks[i-1] + 1) {
                    consecutiveCount++;
                    maxConsecutive = std::max(maxConsecutive, consecutiveCount);
                } else {
                    consecutiveCount = 1;
                }
            }

            if (maxConsecutive >= 3) {
                score += 40;  // 有连续的牌，增加控制能力
            }
        }

        // 3. 评估是否应该保留某些牌
        if (tableLayout.count(s) > 0) {
            int playedCards = tableLayout.at(s).size();
            if (playedCards <= 2) {
                score -= 20;  // 如果这个花色刚开始，可能需要保留一些牌
            }
        }

        return score;
    }
};

// 这个函数是必需的，用于动态加载
extern "C" PlayerStrategy* createStrategy() {
    return new SmartStrategy();
}

} // namespace sevens