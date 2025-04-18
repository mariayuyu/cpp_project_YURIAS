#include "MyCardParser.hpp"
#include <iostream>

namespace sevens {

void MyCardParser::read_cards(const std::string& filename) {
    // TODO: For example, create a standard 52-card deck.
    // Or read from an input file.

    // Example stub:
    std::cout << "[MyCardParser::read_cards] Creating 52-card deck.\n";

    int id = 0; 
    for (int suit = 0; suit < 4; ++suit){
        for (int rank = 1; rank < 14; ++rank){
            this->cards_hashmap[id++] = Card{suit, rank};
        }
    }

}

} // namespace sevens
