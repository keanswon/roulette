#pragma once
#include <string>
#include <stdexcept>

struct Bet {
    enum class Type {
        Straight, Split, Street, Corner, SixLine,
        Column, Dozen, Red, Black, Odd, Even, High, Low
    };

    Type type;
    std::string selection_label;
    double amount;
    double payout_odds;

    Bet(Type t, const std::string& selection, double amount)
    : type(t), selection_label(selection), amount(amount) {

        if (t == Type::Straight) payout_odds = 35.0;
        else if (t == Type::Split) payout_odds = 17.0;
        else if (t == Type::Street) payout_odds = 11.0;
        else if (t == Type::Corner) payout_odds = 8.0;
        else if (t == Type::SixLine) payout_odds = 5.0;
        else if (t == Type::Column || t == Type::Dozen) payout_odds = 2.0;
        else if (t == Type::Red || t == Type::Black ||
                t == Type::Odd || t == Type::Even ||
                t == Type::High || t == Type::Low) payout_odds = 1.0;

        else throw std::invalid_argument("invalid bet type");
    }
};

std::string bet_type_to_string(Bet::Type type) {
    switch (type) {
        case Bet::Type::Straight: return "Straight";
        case Bet::Type::Split:    return "Split";
        case Bet::Type::Street:   return "Street";
        case Bet::Type::Corner:   return "Corner";
        case Bet::Type::SixLine:  return "Six Line";
        case Bet::Type::Column:   return "Column";
        case Bet::Type::Dozen:    return "Dozen";
        case Bet::Type::Red:      return "Red";
        case Bet::Type::Black:    return "Black";
        case Bet::Type::Odd:      return "Odd";
        case Bet::Type::Even:     return "Even";
        case Bet::Type::High:     return "High";
        case Bet::Type::Low:      return "Low";
    }
    return "Unknown";
}