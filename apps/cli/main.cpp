#include <iostream>
#include "roulette.hpp"
#include "player.hpp"
#include "bet.hpp"

/**                     ~~~~ BETTING ~~~~
 * Straight: single number                      -- Payout 35:1
 * Split: two adjacent numbers                  -- Payout 17:1
 * Street: three numbers in a row               -- Payout 11:1
 * Corner: four numbers in a square             -- Payout 8:1
 * SixLine: six numbers in two adjacent rows    -- Payout 5:1
 * Column: one of the three vertical columns    -- Payout 2:1
 * Dozen: one of the three dozens (1-12, 13-24,
 *  25-36)                                      -- Payout 2:1
 * Red/Black: all red or black numbers          -- Payout 1:1
 * Odd/Even: all odd or even numbers            -- Payout 1:1
 * High/Low: 1-18 (Low) or 19-36 (High)         -- Payout 1:1
 * Payout odds are based on standard roulette rules.
 */

 /** for reference */

//  enum class Type {
//         Straight, Split, Street, Corner, SixLine,
//         Column, Dozen, Red, Black, Odd, Even, High, Low
//  };

int main() {
    Wheel w(Wheel::Type::American);
    
    Player player("Alice", 1000.0);
    Bet bet = Bet(Bet::Type::Corner, "14", 50.0);
    player.place_bet(bet);

    std::cout << "Player: " << player.name() << ", Bet: $" << bet.amount
              << " on " << bet_type_to_string(bet.type) << "\n";
    
    // Now spin the wheel
    int idx = w.spin_index();
    const Pocket& p = w.pocket_by_index(idx);    // still needed for color/parity/dozen/column
    std::string hit_label = w.label_by_index(idx); // use this for Straight + display

    std::cout << "Hit: " << hit_label << "\n";
    std::cout << (is_red(p) ? "Red" : is_black(p) ? "Black" : "Green") << "\n";
    if (!is_green(p)) {
        std::cout << (is_even(p) ? "Even" : "Odd") << ", Dozen " << dozen(p)
                  << ", Column " << column(p) << "\n";
    }

    // check if player won
    if (player_won(bet, hit_label, p)) {
        double total_payout = bet.amount + (bet.amount * bet.payout_odds);
        player.credit(total_payout);
        std::cout << "Player wins! Total payout: $" << total_payout
                << " (original bet: $" << bet.amount
                << " + winnings: $" << (bet.amount * bet.payout_odds) << ")\n";
    } else {
        std::cout << "Player loses bet of $" << bet.amount << "\n";
    }

    // print winnings
    std::cout << "Player: " << player.name() << ", Balance: $" << player.balance() << "\n";
}