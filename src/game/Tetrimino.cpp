#include "Tetrimino.h"

static std::array<int,16> rotFrom(std::initializer_list<int> lst){
    std::array<int,16> a{};
    int i=0; for(int v: lst) a[i++] = v;
    return a;
}

Tetromino::Tetromino(TetrominoType t): type(t) {
    if (t == TetrominoType::I) {
        color = sf::Color::Cyan;
        states = { rotFrom({0,0,0,0, 1,1,1,1, 0,0,0,0, 0,0,0,0}), rotFrom({0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0}), rotFrom({0,0,0,0, 0,0,0,0, 1,1,1,1, 0,0,0,0}), rotFrom({0,1,0,0, 0,1,0,0, 0,1,0,0, 0,1,0,0}) };
        numStates = 2;
    } else if (t == TetrominoType::O) {
        color = sf::Color::Yellow;
        states = { rotFrom({0,1,1,0, 0,1,1,0, 0,0,0,0, 0,0,0,0}), rotFrom({0,1,1,0, 0,1,1,0, 0,0,0,0, 0,0,0,0}), rotFrom({0,1,1,0, 0,1,1,0, 0,0,0,0, 0,0,0,0}), rotFrom({0,1,1,0, 0,1,1,0, 0,0,0,0, 0,0,0,0}) };
        numStates = 1;
    } else if (t == TetrominoType::T) {
        color = sf::Color(128,0,128);
        states = { rotFrom({0,1,0,0, 1,1,1,0, 0,0,0,0, 0,0,0,0}), rotFrom({0,1,0,0, 0,1,1,0, 0,1,0,0, 0,0,0,0}), rotFrom({0,0,0,0, 1,1,1,0, 0,1,0,0, 0,0,0,0}), rotFrom({0,1,0,0, 1,1,0,0, 0,1,0,0, 0,0,0,0}) };
        numStates = 4;
    } else if (t == TetrominoType::L) {
        color = sf::Color(255,165,0);
        states = { rotFrom({0,0,1,0, 1,1,1,0, 0,0,0,0, 0,0,0,0}), rotFrom({0,1,0,0, 0,1,0,0, 0,1,1,0, 0,0,0,0}), rotFrom({0,0,0,0, 1,1,1,0, 1,0,0,0, 0,0,0,0}), rotFrom({1,1,0,0, 0,1,0,0, 0,1,0,0, 0,0,0,0}) };
        numStates = 4;
    } else if (t == TetrominoType::J) {
        color = sf::Color::Blue;
        states = { rotFrom({1,0,0,0, 1,1,1,0, 0,0,0,0, 0,0,0,0}), rotFrom({0,1,1,0, 0,1,0,0, 0,1,0,0, 0,0,0,0}), rotFrom({0,0,0,0, 1,1,1,0, 0,0,1,0, 0,0,0,0}), rotFrom({0,1,0,0, 0,1,0,0, 1,1,0,0, 0,0,0,0}) };
        numStates = 4;
    } else if (t == TetrominoType::S) {
        color = sf::Color::Green;
        states = { rotFrom({0,1,1,0, 1,1,0,0, 0,0,0,0, 0,0,0,0}), rotFrom({0,1,0,0, 0,1,1,0, 0,0,1,0, 0,0,0,0}), rotFrom({0,0,0,0, 0,1,1,0, 1,1,0,0, 0,0,0,0}), rotFrom({1,0,0,0, 1,1,0,0, 0,1,0,0, 0,0,0,0}) };
        numStates = 2;
    } else if (t == TetrominoType::Z) {
        color = sf::Color::Red;
        states = { rotFrom({1,1,0,0, 0,1,1,0, 0,0,0,0, 0,0,0,0}), rotFrom({0,0,1,0, 0,1,1,0, 0,1,0,0, 0,0,0,0}), rotFrom({0,0,0,0, 1,1,0,0, 0,1,1,0, 0,0,0,0}), rotFrom({0,1,0,0, 1,1,0,0, 1,0,0,0, 0,0,0,0}) };
        numStates = 2;
    }
}

const int* Tetromino::state(int rotation) const {
    return states[rotation % numStates].data();
}