#include "PlayerStrategy.hpp"
#include <algorithm>
#include <vector>
#include <string>
#include <random>
#include <chrono>

namespace sevens {

/**
 * Template for student strategy implementation.
 * To create your own strategy:
 * 1. Copy this file to a new file (e.g., MyStrategy.cpp)
 * 2. Rename StudentStrategy to your strategy name
 * 3. Implement all the virtual methods
 * 4. Keep the extern "C" createStrategy() function
 */
class StudentStrategy : public PlayerStrategy {
public:
    StudentStrategy() {
        auto seed = static_cast<unsigned long>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }
    
    ~StudentStrategy() override = default;
    
    void initialize(uint64_t playerID) override {
        myID = playerID;
        // TODO: any further initialization
    }
    
    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        // TODO: implement logic
        // Return index in hand for the card to play, or -1 if pass
        return -1;
    }
    
    void observeMove(uint64_t playerID, const Card& playedCard) override {
        // TODO: track other players' moves if you need
        (void)playerID;
        (void)playedCard;
    }
    
    void observePass(uint64_t playerID) override {
        // TODO: track passes if you need
        (void)playerID;
    }
    
    std::string getName() const override {
        return "StudentTemplate";
    }

private:
    uint64_t myID;
    std::mt19937 rng;
};

// This function is required for dynamic loading
extern "C" PlayerStrategy* createStrategy() {
    return new StudentStrategy();
}

} // namespace sevens
