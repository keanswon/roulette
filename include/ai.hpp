#pragma once
#include "player.hpp"
#include "bet.hpp"



class AI {
public:
    AI() = default;

    // Function to decide the bet type based on some strategy
    Bet decide_bet(const Player& player) {
        // Example logic: always bet on black if player has enough balance
        if (player.balance() >= 50.0) {
            return Bet(Bet::Type::Black, "", 50.0);
        }
        // Otherwise, place a minimum bet on red
        return Bet(Bet::Type::Red, "", 10.0);
    }    
};