// roulette.hpp
#pragma once
#include <array>
#include <random>
#include <string_view>
#include <cstdint>
#include <cctype>
#include <string>
#include "bet.hpp"


// use binary attributes : computations are quicker
enum Attr : uint16_t {
    RED     = 1<<0,
    BLACK   = 1<<1,
    GREEN   = 1<<2,
    ODD     = 1<<3,
    EVEN    = 1<<4,
    D1      = 1<<5,   // 1–12
    D2      = 1<<6,   // 13–24
    D3      = 1<<7,   // 25–36
    C1      = 1<<8,   // first column
    C2      = 1<<9,   // second column
    C3      = 1<<10   // third column
};

struct Pocket {
    int value;           
    bool isDoubleZero;   
    uint16_t attrs;      
};

constexpr bool is_red_number(int n) {
    switch (n) {
        case 1: case 3: case 5: case 7: case 9: 
        case 12: case 14: case 16: case 18: case 19:
        case 21: case 23: case 25: case 27: case 30:
        case 32: case 34: case 36:
            return true;
        default:
            return false;
    }
}

constexpr uint16_t build_attrs(int n, bool is00) {
    if (n == 0 || is00) return GREEN;
    uint16_t a = 0;
    a |= is_red_number(n) ? RED : BLACK;
    a |= (n % 2 ? ODD : EVEN);
    if (n >= 1 && n <= 12)      a |= D1;
    else if (n <= 24)           a |= D2;
    else                        a |= D3;
    int col = (n - 1) % 3;
    a |= (col == 0 ? C1 : (col == 1 ? C2 : C3));
    return a;
}

// ---------- Tables ----------
inline const std::array<Pocket, 37> EURO_TABLE = []{
    std::array<Pocket, 37> t{};
    t[0] = Pocket{0, false, build_attrs(0, false)};
    for (int i = 1; i <= 36; ++i) t[i] = Pocket{i, false, build_attrs(i, false)};
    return t;
}();

inline const std::array<Pocket, 38> AMERICAN_TABLE = []{
    std::array<Pocket, 38> t{};
    t[0] = Pocket{0, false, build_attrs(0, false)};
    for (int i = 1; i <= 36; ++i) t[i] = Pocket{i, false, build_attrs(i, false)};
    t[37] = Pocket{0, true, build_attrs(0, true)}; // "00"
    return t;
}();


inline constexpr int EURO_WHEEL_ORDER[37] = {
    0,32,15,19,4,21,2,25,17,34,6,27,13,36,11,30,8,23,10,
    5,24,16,33,1,20,14,31,9,22,18,29,7,28,12,35,3,26
};
inline constexpr int AMER_WHEEL_ORDER[38] = {
    0,28,9,26,30,11,7,20,32,17,5,22,34,15,3,24,36,13,1,
    0,27,10,25,29,12,8,19,31,18,6,21,33,16,4,23,35,14,2
};

// Map number to American table index (handles 00)
inline int american_value_to_index(int value, bool isDoubleZero=false) {
    if (isDoubleZero) return 37;     // 00
    return value;                    // 0..36 map directly
}

inline int european_value_to_index(int value) {
    return value;                    // 0..36 map directly
}

// Pretty label
inline std::string pocket_label(const Pocket& p) {
    return (p.value == 0 && p.isDoubleZero) ? "00" :
           (p.value == 0 ? "0" : std::to_string(p.value));
}

// ---------- Queries ----------
inline bool has(uint16_t attrs, Attr flag) { return (attrs & flag) != 0; }
inline bool is_grid_number(int n) { return 1 <= n && n <= 36; }
inline bool is_red(const Pocket& p)   { return has(p.attrs, RED);   }
inline bool is_black(const Pocket& p) { return has(p.attrs, BLACK); }
inline bool is_green(const Pocket& p) { return has(p.attrs, GREEN); }
inline bool is_even(const Pocket& p)  { return has(p.attrs, EVEN);  }
inline bool is_odd (const Pocket& p)  { return has(p.attrs, ODD);   }
inline int  dozen   (const Pocket& p) { return has(p.attrs,D1)?1:has(p.attrs,D2)?2:has(p.attrs,D3)?3:0; }
inline int  column  (const Pocket& p) { return has(p.attrs,C1)?1:has(p.attrs,C2)?2:has(p.attrs,C3)?3:0; }

static inline bool parse_number_label(const std::string& s, int& out) {
    if (s == "0" || s == "00") return false;             // greens aren't part of layout combos
    if (s.empty()) return false;
    for (unsigned char c : s) if (!std::isdigit(c)) return false;
    int v = std::stoi(s);
    if (v < 1 || v > 36) return false;
    out = v; return true;
}

static inline bool split_pair(const std::string& s, int& a, int& b) {
    size_t dash = s.find('-');
    if (dash == std::string::npos) return false;
    std::string s1 = s.substr(0, dash), s2 = s.substr(dash + 1);
    return parse_number_label(s1, a) && parse_number_label(s2, b);
}

bool player_won(const Bet& bet, const std::string& hit_label, const Pocket& p) {
    switch (bet.type) {
        case Bet::Type::Straight:
            return hit_label == bet.selection_label;

        case Bet::Type::Red:    return is_red(p);
        case Bet::Type::Black:  return is_black(p);
        case Bet::Type::Odd:    return !is_green(p) && !is_even(p);
        case Bet::Type::Even:   return !is_green(p) &&  is_even(p);
        case Bet::Type::High:   return !is_green(p) && (dozen(p) == 2 || dozen(p) == 3);
        case Bet::Type::Low:    return !is_green(p) && (dozen(p) == 1);
        case Bet::Type::Dozen:  return !is_green(p) && std::to_string(dozen(p))  == bet.selection_label;
        case Bet::Type::Column: return !is_green(p) && std::to_string(column(p)) == bet.selection_label;

        case Bet::Type::Split: {
            if (is_green(p)) return false;
            int hit; if (!parse_number_label(hit_label, hit)) return false;

            int a, b;
            if (split_pair(bet.selection_label, a, b))
                return hit == a || hit == b;

            if (bet.selection_label.size() < 2) return false;
            char orient = bet.selection_label.back();
            std::string s = bet.selection_label.substr(0, bet.selection_label.size()-1);
            int anchor; if (!parse_number_label(s, anchor)) return false;
            if (orient == 'H' || orient == 'h') {
                return hit == anchor || hit == anchor + 1;
            } else {
                return hit == anchor || hit == anchor + 3;
            }
        }

        case Bet::Type::Street: {
            if (is_green(p)) return false;
            int anchor, hit; 
            if (!parse_number_label(bet.selection_label, anchor)) return false;
            if (!parse_number_label(hit_label, hit)) return false;
            return hit == anchor || hit == anchor + 1 || hit == anchor + 2;
        }

        case Bet::Type::Corner: {
            if (is_green(p)) return false;
            int anchor, hit; 
            if (!parse_number_label(bet.selection_label, anchor)) return false;
            if (!parse_number_label(hit_label, hit)) return false;
            return hit == anchor || hit == anchor + 1 ||
                   hit == anchor + 3 || hit == anchor + 4;
        }

        case Bet::Type::SixLine: {
            if (is_green(p)) return false;
            int anchor, hit; 
            if (!parse_number_label(bet.selection_label, anchor)) return false;
            if (!parse_number_label(hit_label, hit)) return false;
            return hit >= anchor && hit <= anchor + 5;
        }
    }
    return false;
}


// ---------- RNG & spin ----------
class Wheel {
public:
    enum class Type { European, American };

    explicit Wheel(Type type, uint64_t seed = std::random_device{}())
    : type_(type), rng_(seed) {}

    int spin_index() {
        if (type_ == Type::European) {
            std::uniform_int_distribution<int> dist(0, 36);
            return dist(rng_);
        } else {
            std::uniform_int_distribution<int> dist(0, 37);
            return dist(rng_);
        }
    }

    const Pocket& pocket_by_index(int idx) const {
        return (type_ == Type::European) ? EURO_TABLE[idx] : AMERICAN_TABLE[idx];
    }

    std::string label_by_index(int idx) const {
        const auto& p = pocket_by_index(idx);
        return std::string(pocket_label(p));
    }

private:
    Type type_;
    std::mt19937_64 rng_;
};
