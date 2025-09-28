#include <SFML/Graphics.hpp>
#include <fstream>
#include <random>
#include <string>
#include <sstream>
#include <algorithm>
#include "game/Board.h"
#include "game/Tetrimino.h"
#include "neat/NEAT.h"

class Bag {
    std::vector<TetrominoType> bag;
    std::mt19937 rng;
public:
    Bag(int seed=123): rng(seed) { refill(); }
    void refill(){
        bag = {TetrominoType::I, TetrominoType::O, TetrominoType::T, TetrominoType::L, TetrominoType::J, TetrominoType::S, TetrominoType::Z};
        std::shuffle(bag.begin(), bag.end(), rng);
    }
    TetrominoType next(){
        if(bag.empty()) refill();
        TetrominoType t = bag.back(); bag.pop_back();
        return t;
    }
};

neat::Genome deserialize_genome_from_string(const std::string& s) {
    neat::Genome g;
    std::stringstream ss(s);
    g.deserialize(ss);
    return g;
}

void drawBlock(sf::RenderWindow& window, float x, float y, float size, const sf::Color& color) {
    sf::RectangleShape block(sf::Vector2f(size, size));
    block.setFillColor(color);
    block.setPosition(x, y);
    sf::Color light = color + sf::Color(50, 50, 50);
    sf::Color dark = color - sf::Color(50, 50, 50);
    sf::Vertex top[] = {sf::Vertex(sf::Vector2f(x,y), light), sf::Vertex(sf::Vector2f(x+size,y), light)};
    sf::Vertex left[] = {sf::Vertex(sf::Vector2f(x,y), light), sf::Vertex(sf::Vector2f(x,y+size), light)};
    sf::Vertex bottom[] = {sf::Vertex(sf::Vector2f(x,y+size), dark), sf::Vertex(sf::Vector2f(x+size,y+size), dark)};
    sf::Vertex right[] = {sf::Vertex(sf::Vector2f(x+size,y), dark), sf::Vertex(sf::Vector2f(x+size,y+size), dark)};
    window.draw(block);
    window.draw(top, 2, sf::Lines); window.draw(left, 2, sf::Lines);
    window.draw(bottom, 2, sf::Lines); window.draw(right, 2, sf::Lines);
}

void drawTetrimino(sf::RenderWindow& window, const Tetromino& tet, int rot, float px, float py, float baseX, float baseY, float CELL_SIZE) {
    const int* s = tet.state(rot);
    for(int by = 0; by < 4; ++by) {
        for(int bx = 0; bx < 4; ++bx) {
            if(s[by * 4 + bx]) {
                drawBlock(window, baseX + (px + bx) * CELL_SIZE, baseY + (py + by) * CELL_SIZE, CELL_SIZE -1, tet.color);
            }
        }
    }
}

int main(){
    std::ifstream in("saved_genome.txt");
    if(!in.is_open()){ std::cerr<<"saved_genome.txt not found.\n"; return 1; }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    neat::Genome g = deserialize_genome_from_string(content);

    const float CELL_SIZE = 25.f, BORDER = 20.f, UI_W = 200.f;
    sf::RenderWindow window(sf::VideoMode(Board::WIDTH*CELL_SIZE+UI_W+2*BORDER, Board::HEIGHT*CELL_SIZE+2*BORDER), "Tetris NEAT Demo");
    window.setFramerateLimit(60);

    sf::Font font; font.loadFromFile("Arial.ttf");
    Bag bag(1234);
    Tetromino current = bag.next(), next = bag.next();
    long score = 0; int totalLines = 0, level = 1;
    float speed = 1.0f;
    Board board;

    while(window.isOpen()){
        sf::Event ev;
        while(window.pollEvent(ev)){
            if(ev.type==sf::Event::Closed) window.close();
            if(ev.type == sf::Event::KeyPressed) {
                if(ev.key.code == sf::Keyboard::Up) speed *= 1.5f;
                else if(ev.key.code == sf::Keyboard::Down) speed /= 1.5f;
            }
        }
        
        if (board.isGameOver()) {
            board.clear(); score = 0; totalLines = 0; level = 1;
        }

        auto placements = board.allPossiblePlacements(current);
        if(placements.empty()){ board.clear(); continue; }
        
        double bestScore = -1e9; int bestIdx = -1;
        for(size_t i=0; i<placements.size(); ++i){
            auto &p = placements[i];
            std::vector<double> inputs = { (double)p.aggregateHeight/400.0, (double)p.holes/400.0, (double)p.bumpiness/400.0, (double)p.clearedLines/4.0 };
            double scoreVal = g.evaluate(inputs);
            if(scoreVal > bestScore){ bestScore = scoreVal; bestIdx = i; }
        }
        
        if(bestIdx < 0) { board.clear(); continue; }

        Placement chosen = placements[bestIdx];
        float animY = -4.0f;
        while(animY < chosen.y) {
            animY += 0.5f * speed;
            if(animY > chosen.y) animY = chosen.y;
            
            window.clear(sf::Color(30, 30, 40));

            for(int y=0; y<Board::HEIGHT; ++y) for (int x=0; x<Board::WIDTH; ++x) {
                sf::RectangleShape cell(sf::Vector2f(CELL_SIZE-1, CELL_SIZE-1));
                cell.setPosition(BORDER + x * CELL_SIZE, BORDER + y * CELL_SIZE);
                cell.setFillColor(sf::Color(40,40,50));
                window.draw(cell);
            }
            
            const auto& cells = board.cells();
            for(int y=0;y<Board::HEIGHT;++y) for(int x=0;x<Board::WIDTH;++x){
                if(cells[y*Board::WIDTH + x]) {
                   drawBlock(window, BORDER + x*CELL_SIZE, BORDER + y*CELL_SIZE, CELL_SIZE-1, sf::Color(128,128,128));
                }
            }

            drawTetrimino(window, current, chosen.rotation, chosen.x, animY, BORDER, BORDER, CELL_SIZE);
            
            sf::Text nextText("NEXT", font, 24);
            nextText.setPosition(Board::WIDTH * CELL_SIZE + 2 * BORDER, BORDER);
            window.draw(nextText);
            drawTetrimino(window, next, 0, Board::WIDTH + 2.5, 2.5, BORDER, BORDER, CELL_SIZE);

            sf::Text scoreText("SCORE\n" + std::to_string(score), font, 24);
            scoreText.setPosition(Board::WIDTH*CELL_SIZE+2*BORDER, 150);
            window.draw(scoreText);

            sf::Text linesText("LINES\n" + std::to_string(totalLines), font, 24);
            linesText.setPosition(Board::WIDTH*CELL_SIZE+2*BORDER, 250);
            window.draw(linesText);

            sf::Text levelText("LEVEL\n" + std::to_string(level), font, 24);
            levelText.setPosition(Board::WIDTH*CELL_SIZE+2*BORDER, 350);
            window.draw(levelText);
            
            window.display();
        }

        board.applyPlacement(chosen, current);
        int cleared = chosen.clearedLines;
        if(cleared > 0) {
            totalLines += cleared;
            level = 1 + totalLines / 10;
            long points[] = {0, 100, 300, 500, 800};
            score += points[cleared] * level;
        }

        current = next;
        next = bag.next();
    }
    return 0;
}