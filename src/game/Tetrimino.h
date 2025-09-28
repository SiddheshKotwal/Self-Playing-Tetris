#pragma once
#include <array>
#include <vector>
#include <SFML/Graphics.hpp>

enum class TetrominoType { I, O, T, L, J, S, Z };

struct Tetromino {
    TetrominoType type;
    std::array<std::array<int,16>,4> states;
    int numStates;
    sf::Color color;

    Tetromino() = default;
    Tetromino(TetrominoType t);
    const int* state(int rotation) const;
    int size() const { return 4; }
};