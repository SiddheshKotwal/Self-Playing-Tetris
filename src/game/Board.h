#pragma once
#include <vector>
#include <optional>
#include "Tetrimino.h"

struct Placement {
    int rotation;
    int x;
    int y;
    int clearedLines;
    int aggregateHeight;
    int holes;
    int bumpiness;
};

class Board {
public:
    static constexpr int WIDTH = 16;
    static constexpr int HEIGHT = 22;
    Board();
    void clear();
    bool isInside(int x,int y) const;
    bool isCell(int x,int y) const;
    bool collides(const Tetromino& tet, int rot, int px, int py) const;
    void lock(const Tetromino& tet, int rot, int px, int py);
    int clearLines();
    void addGarbageLine();
    Placement evaluatePlacement(const Tetromino& tet, int rot, int px) const;
    void applyPlacement(const Placement& pl, const Tetromino& tet);
    bool isGameOver() const;
    std::vector<Placement> allPossiblePlacements(const Tetromino& tet) const;
    const std::vector<int>& cells() const { return grid; }
private:
    std::vector<int> grid;
    int colHeight(int x) const;
    int computeHoles() const;
};