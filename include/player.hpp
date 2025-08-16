#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include "bet.hpp"

class Player {
public:
    Player(std::string name, double balance)
        : name_(std::move(name)), balance_(balance) {}

    const std::string& name() const { return name_; }
    double balance() const { return balance_; }

    void credit(double amount) { balance_ += amount; }
    void debit(double amount) {
        if (amount > balance_) throw std::runtime_error("Insufficient funds");
        balance_ -= amount;
    }

    void place_bet(const Bet& bet) {
        debit(bet.amount);   // deduct immediately
        bets_.push_back(bet);
    }

    Bet get_bet(size_t index) const {
        if (index < 0 || index >= bets_.size()) {
            throw std::out_of_range("Bet index out of range");
        }
        return bets_[index];
    }

    void clear_bets() { bets_.clear(); }

    const std::vector<Bet>& bets() const { return bets_; }

private:
    std::string name_;
    double balance_;
    std::vector<Bet> bets_;
};
