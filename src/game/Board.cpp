#include "Board.h"
#include <algorithm>
#include <limits>
#include <cstring>
#include <random>
#include <chrono>

Board::Board(): grid(WIDTH*HEIGHT, 0) {}

void Board::clear(){ std::fill(grid.begin(), grid.end(), 0); }

bool Board::isInside(int x,int y) const {
    return x>=0 && x<WIDTH && y>=0 && y<HEIGHT;
}

bool Board::isCell(int x,int y) const {
    if(!isInside(x,y)) return false;
    return grid[y*WIDTH + x] != 0;
}

void Board::addGarbageLine() {
    for (int y = 0; y < HEIGHT - 1; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            grid[y * WIDTH + x] = grid[(y + 1) * WIDTH + x];
        }
    }
    std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist(0, WIDTH - 1);
    int hole_x = dist(rng);
    for (int x = 0; x < WIDTH; ++x) {
        grid[(HEIGHT - 1) * WIDTH + x] = (x == hole_x) ? 0 : 1;
    }
}

bool Board::collides(const Tetromino& tet, int rot, int px, int py) const {
    const int* s = tet.state(rot);
    int sz = tet.size();
    for(int by=0; by<sz; ++by){
        for(int bx=0; bx<sz; ++bx){
            if(!s[by*sz + bx]) continue;
            int gx = px + bx;
            int gy = py + by;
            if(gx < 0 || gx >= WIDTH || gy >= HEIGHT) return true;
            if(gy >= 0 && isCell(gx, gy)) return true;
        }
    }
    return false;
}

void Board::lock(const Tetromino& tet, int rot, int px, int py) {
    const int* s = tet.state(rot);
    int sz = tet.size();
    for(int by=0; by<sz; ++by){
        for(int bx=0; bx<sz; ++bx){
            if(s[by*sz + bx]){
                int gx = px + bx;
                int gy = py + by;
                if(isInside(gx, gy)) {
                    grid[gy*WIDTH + gx] = 1;
                }
            }
        }
    }
}

int Board::clearLines(){
    int cleared = 0;
    int write_y = HEIGHT-1;
    for(int read_y=HEIGHT-1; read_y>=0; --read_y){
        bool full = true;
        for(int x=0;x<WIDTH;++x) if(!isCell(x,read_y)){ full=false; break; }
        if(full) {
            ++cleared;
        } else {
            if(write_y != read_y){
                for(int x=0;x<WIDTH;++x) grid[write_y*WIDTH + x] = grid[read_y*WIDTH + x];
            }
            --write_y;
        }
    }
    for(int y=write_y; y>=0; --y) {
        for(int x=0;x<WIDTH;++x) grid[y*WIDTH + x] = 0;
    }
    return cleared;
}

int Board::colHeight(int x) const {
    for(int y=0;y<HEIGHT;++y){
        if(isCell(x,y)) return HEIGHT - y;
    }
    return 0;
}

int Board::computeHoles() const {
    int holes = 0;
    for(int x=0;x<WIDTH;++x){
        bool blockSeen = false;
        for(int y=0;y<HEIGHT;++y){
            if(isCell(x,y)) blockSeen = true;
            else if(blockSeen) ++holes;
        }
    }
    return holes;
}

Placement Board::evaluatePlacement(const Tetromino& tet, int rot, int px) const {
    Board b = *this;
    int py = -4;
    for(int testY = -4; testY<HEIGHT; ++testY){
        if(b.collides(tet, rot, px, testY)){
            py = testY - 1;
            break;
        }
    }
    if (collides(tet, rot, px, -2)) { // check for spawn collision
        Placement p; p.aggregateHeight = 9999; return p;
    }

    b.lock(tet, rot, px, py);
    int cleared = b.clearLines();
    int aggH = 0;
    std::array<int, WIDTH> heights{};
    for(int x=0;x<WIDTH;++x){ heights[x] = b.colHeight(x); aggH += heights[x]; }
    int holes = b.computeHoles();
    int bump = 0;
    for(int x=0;x<WIDTH-1;++x) bump += std::abs(heights[x] - heights[x+1]);
    
    return {rot, px, py, cleared, aggH, holes, bump};
}

void Board::applyPlacement(const Placement& pl, const Tetromino& tet){
    lock(tet, pl.rotation, pl.x, pl.y);
    clearLines();
}

bool Board::isGameOver() const {
    for(int x=0;x<WIDTH;++x) if(isCell(x,0)) return true;
    return false;
}

std::vector<Placement> Board::allPossiblePlacements(const Tetromino& tet) const {
    std::vector<Placement> out;
    for(int r=0; r<tet.numStates; ++r){
        for(int px = -3; px < WIDTH; ++px){
            Placement pl = evaluatePlacement(tet, r, px);
            if(pl.aggregateHeight >= 9999) continue;
            out.push_back(pl);
        }
    }
    return out;
}