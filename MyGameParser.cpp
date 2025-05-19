#include "MyGameParser.hpp"
#include <iostream>

namespace sevens {

void MyGameParser::read_game(const std::string& filename) {
    // TODO: e.g., set table_layout[suit][rank] = (rank == 7) for the start of Sevens
    std::cout << "[MyGameParser::read_game] Setting up the table.\n";

    for (uint64_t suit = 0; suit < 4; ++suit){
        table_layout[suit][7] = true;
    }
}

void MyGameParser::read_cards(const std::string& filename) {
    // You can leave it empty if not used
    std::cout << "[MyGameParser::read_cards] No cards to load in this parser.\n";
}

} // namespace sevens
