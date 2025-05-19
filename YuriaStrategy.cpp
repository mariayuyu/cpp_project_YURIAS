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
        // initialize random number generator
        auto seed = static_cast<unsigned long>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }

    ~YuriaStrategy() override = default;

    // called once at the start of the game to initialize variables
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

    // called every turn to select the best card to play or returns -1 to pass
    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override
    {
        round++;
        updateGamePhase(); // update game phase (early/mid/late) based on total cards played
        
        std::cout << "\n[Turn " << round << "] Player " << myID << " selecting card...\n";
        std::cout << "  Current hand: ";
        for (const auto& c : hand) std::cout << c << " ";
        std::cout << "\n";

        std::vector<std::pair<int, int>> candidates; // {card index, score}
        std::unordered_map<int, int> suitCount;     // count cards of each suit in hand
        for (const auto& c : hand) suitCount[c.suit]++;
        
        // Update suit playability based on the current table layout
        updateSuitPlayability(tableLayout);

        for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
            const Card& card = hand[i];
            if (!isPlayable(card, tableLayout)) continue;
            
            int score = evaluateCard(card, hand, tableLayout, suitCount);
            
            // show how the card was evaluated
            std::cout << "  -> Candidate " << card
                      << " | score=" << score 
                      << " | " << getEvaluationDetails(card, hand, tableLayout, suitCount) << "\n";

            candidates.emplace_back(i, score);
        }

        // No playable cards -> passing instead
        if (candidates.empty()) {
            std::cout << "  -> No playable cards. Passing.\n";
            return -1;
        }

        // sort cards by score descending on and choose the highest scoring one
        std::sort(candidates.begin(), candidates.end(),
                 [](auto& a, auto& b) { return a.second > b.second; });

        const Card& chosen = hand[candidates.front().first];
        std::cout << "  -> Playing: " << chosen << " (score=" << candidates.front().second << ")\n";
        return candidates.front().first;
    }

    // called when another player successfully plays a card
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        playedCards[playedCard.suit].insert(playedCard.rank);
        passCounts[playerID] = 0;   // reset pass count for this player
        
        // if a player plays a 7, mark that suit as highly playable
        if (playedCard.rank == 7) {
            suitPlayability[playedCard.suit] = 2; 
        }
        
        std::cout << "[ObserveMove] Player " << playerID << " played " << playedCard << "\n";
    }

    // called when another player passes
    void observePass(uint64_t playerID) override {
        passCounts[playerID]++;
        
        // if a player passes twice, we suspect they're blocked in a suit
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
    // maps suits to the set of ranks that have already been played
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> playedCards;
    // number of consecutive passes per player
    std::unordered_map<uint64_t, int> passCounts;
    // estimated probability that a player is blocked (based on passes)
    std::unordered_map<uint64_t, int> blockProbabilities;
    // estimated playability of each suit 0=low 1=medium 2=high
    std::unordered_map<int, int> suitPlayability;
    
    // Game phase flags
    bool isEarlyGame;
    bool isMidGame;
    bool isLateGame;

    // determine the current game phase based on how many cards have been played
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
    
    // evaluate the playability of each suit using table layout
    void updateSuitPlayability(const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) {
        for (int suit = 1; suit <= 4; suit++) {
            if (!tableLayout.count(suit)) {
                suitPlayability[suit] = 0; // suit not yet opened
                continue;
            }
            
            const auto& layout = tableLayout.at(suit);
            int openEnds = 0;       // number of ranks with one neighbor still unplayed
            int coveredRanks = 0;   // how many cards in this suit are already on the table
            
            for (int r = 1; r <= 13; r++) {
                if (layout.count(r) && layout.at(r)) {
                    coveredRanks++;
                    
                    // check if it's an open end
                    if ((r > 1 && (!layout.count(r-1) || !layout.at(r-1))) ||
                        (r < 13 && (!layout.count(r+1) || !layout.at(r+1)))) {
                        openEnds++;
                    }
                }
            }
            
            // assign playability level based on open ends
            if (openEnds >= 3) suitPlayability[suit] = 2; 
            else if (openEnds >= 1) suitPlayability[suit] = 1; 
            else suitPlayability[suit] = 0; 
            
            // if 10+ cards are played in a suit, consider it highly active
            if (coveredRanks >= 10) suitPlayability[suit] = 2;
        }
    }
    
    // Compute a score for a card based on various tactical factors
    int evaluateCard(const Card& card, const std::vector<Card>& hand, 
                     const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
                     const std::unordered_map<int, int>& suitCount) {
        int score = 0;
        bool is7 = (card.rank == 7);
        bool isEdge = (card.rank == 1 || card.rank == 13);
        bool hasNeighbor = hasChain(card, hand);
        bool risky = opensBothEnds(card, tableLayout);
        int chainLength = calculateChainLength(card, hand);
        
        // Reward playing 7s
        if (is7) score += 200;  
        
        // Adjust scoring by game phase
        if (isEarlyGame) {
            if (is7) score += 100;
            score += chainLength * 30;
            if (hasNeighbor) score += 70;
            if (isEdge) score += 20;
        } else if (isMidGame) {
            score += chainLength * 50;
            if (suitCount.at(card.suit) >= 3) score += 60;
            if (isEdge) score += 40;
        } else { // late game
            score += suitCount.at(card.suit) * 50;
            score += (13 - static_cast<int>(playedCards[card.suit].size())) * 10;
        }
        
        score += suitPlayability[card.suit] * 40;
        
        // penalize cards that open both ends (create many new options for other players)
        if (risky) score -= 80;
        
        // Bonus if opponents are likely blocked
        int blockedPlayers = 0;
        for (const auto& [pid, count] : blockProbabilities) {
            if (count >= 2) blockedPlayers++;
        }
        if (blockedPlayers > 0) score += 30;
        
        // Bonus for strong edge card combos
        if (isEdge) {
            if (card.rank == 1 && hasCardWithRank(hand, card.suit, 2)) score += 80;
            if (card.rank == 13 && hasCardWithRank(hand, card.suit, 12)) score += 80;
        }
        
        return score;
    }
    
    // build a string to explain how a card's score was computed
    std::string getEvaluationDetails(const Card& card, const std::vector<Card>& hand, 
                                     const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
                                     const std::unordered_map<int, int>& suitCount) {
        std::string details;
        bool is7 = (card.rank == 7);
        bool isEdge = (card.rank == 1 || card.rank == 13);
        bool hasNeighbor = hasChain(card, hand);
        bool risky = opensBothEnds(card, tableLayout);
        int chainLength = calculateChainLength(card, hand);
        
        details = "Phase=" + std::string(isEarlyGame ? "Early" : (isMidGame ? "Mid" : "Late"));
        details += " | Chain=" + std::to_string(chainLength);
        details += " | Suit=" + std::to_string(card.suit) + "(play=" + std::to_string(suitPlayability[card.suit]) + ")";
        if (is7) details += " | Seven";
        if (isEdge) details += " | Edge";
        if (hasNeighbor) details += " | HasNeighbor";
        if (risky) details += " | Risky";
        
        return details;
    }

    // Check if a card can be legally played given the current table layout
    bool isPlayable(const Card& card,
                    const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) {
        if (!tableLayout.count(card.suit)) {
            return card.rank == 7;
        }
        
        const auto& layout = tableLayout.at(card.suit);
        if (card.rank == 7) return !layout.count(7) || !layout.at(7);
        
        bool canPlayLower = card.rank > 1 && layout.count(card.rank - 1) && layout.at(card.rank - 1);
        bool canPlayHigher = card.rank < 13 && layout.count(card.rank + 1) && layout.at(card.rank + 1);
        
        return (canPlayLower || canPlayHigher) && (!layout.count(card.rank) || !layout.at(card.rank));
    }
    
    // Check if we have a card that can form a chain with this one
    bool hasChain(const Card& card, const std::vector<Card>& hand) {
        return hasCardWithRank(hand, card.suit, card.rank - 1) ||
               hasCardWithRank(hand, card.suit, card.rank + 1);
    }
    
    // Check if playing this card would create playable positions on both sides
    bool opensBothEnds(const Card& card,
                        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) {
        if (!tableLayout.count(card.suit)) return false;
        const auto& layout = tableLayout.at(card.suit);
        
        bool hasLower = card.rank > 1 && layout.count(card.rank - 1) && layout.at(card.rank - 1);
        bool hasHigher = card.rank < 13 && layout.count(card.rank + 1) && layout.at(card.rank + 1);
        
        return hasLower && hasHigher;
    }
    
    // Calculate the length of the chain of cards we could play
    int calculateChainLength(const Card& card, const std::vector<Card>& hand) {
        int length = 1;
        uint64_t rank = card.rank;
        
        // count cards below
        while (rank > 1 && hasCardWithRank(hand, card.suit, rank - 1)) {
            length++;
            rank--;
        }
        
        // count cards above
        rank = card.rank;
        while (rank < 13 && hasCardWithRank(hand, card.suit, rank + 1)) {
            length++;
            rank++;
        }
        
        return length;
    }
    
    // Check if we have a card of the given suit and rank in our hand
    bool hasCardWithRank(const std::vector<Card>& hand, uint64_t suit, uint64_t rank) {
        if (rank < 1 || rank > 13) return false;
        for (const auto& c : hand) {
            if (static_cast<uint64_t>(c.suit) == suit && static_cast<uint64_t>(c.rank) == rank) {
                return true;
            }
        }
        return false;
    }
};

// Factory function to create a new strategy instance
extern "C" PlayerStrategy* createStrategy() {
    return new YuriaStrategy();
}

} // namespace sevens
