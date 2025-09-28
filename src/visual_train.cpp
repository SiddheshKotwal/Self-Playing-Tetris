#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <thread>
#include <future>
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

int linesClearedInGame(const neat::Genome &g, int seed){
    Board b;
    Bag bag(seed);
    int totalLines = 0;
    int pieceCount = 0;
    const int maxPieces = 500;
    const int garbageFrequency = 25;

    auto makeTet = [](TetrominoType t){ return Tetromino(t); };
    while(pieceCount < maxPieces){
        Tetromino tet(makeTet(bag.next()));
        auto placements = b.allPossiblePlacements(tet);
        if(placements.empty()) break;
        double bestScore = -1e9;
        int bestIdx = -1;
        for(size_t i=0;i<placements.size();++i){
            auto &p = placements[i];
            std::vector<double> inputs = { (double)p.aggregateHeight/400.0, (double)p.holes/400.0, (double)p.bumpiness/400.0, (double)p.clearedLines/4.0 };
            double score = g.evaluate(inputs);
            if(score > bestScore){ bestScore = score; bestIdx = i; }
        }
        if(bestIdx<0) break;
        
        b.applyPlacement(placements[bestIdx], tet);
        totalLines += placements[bestIdx].clearedLines;
        pieceCount++;

        if (pieceCount > 0 && pieceCount % garbageFrequency == 0) {
            b.addGarbageLine();
        }

        if(b.isGameOver()) break;
    }
    return totalLines;
}

void evaluate_genome_fitness(neat::Genome &g, int gen){
    int fitness = 0;
    for(int s=0; s<3; ++s){
        fitness += linesClearedInGame(g, gen*10000 + g.nodes[0].id * 10 + s);
    }
    g.fitness = fitness;
}

void drawTetriminoVis(sf::RenderWindow& window, const Tetromino& tet, int rot, float px, float py, float baseX, float baseY, float CELL_SIZE, sf::Color color) {
    const int* s = tet.state(rot);
    for(int by = 0; by < 4; ++by) {
        for(int bx = 0; bx < 4; ++bx) {
            if(s[by * 4 + bx]) {
                sf::RectangleShape r(sf::Vector2f(CELL_SIZE - 1, CELL_SIZE - 1));
                r.setPosition(baseX + (px + bx) * CELL_SIZE, baseY + (py + by) * CELL_SIZE);
                r.setFillColor(color);
                window.draw(r);
            }
        }
    }
}

void visualizeGame(sf::RenderWindow& window, const neat::Genome& g, sf::Font& font, int generation, double bestFitness) {
    const float CELL_SIZE = 20.f, BORDER = 20.f;
    Board board;
    Bag bag(12345);
    int totalLines = 0;
    Tetromino current = Tetromino(bag.next());
    
    while(window.isOpen() && !board.isGameOver()) {
        sf::Event ev;
        while(window.pollEvent(ev)){
            if(ev.type==sf::Event::Closed) window.close();
        }

        auto placements = board.allPossiblePlacements(current);
        if(placements.empty()) break;

        double bestScore = -1e9;
        int bestIdx = -1;
        for(size_t i=0;i<placements.size();++i){
            auto &p = placements[i];
            std::vector<double> inputs = { (double)p.aggregateHeight/400.0, (double)p.holes/400.0, (double)p.bumpiness/400.0, (double)p.clearedLines/4.0 };
            double score = g.evaluate(inputs);
            if(score > bestScore){ bestScore = score; bestIdx = i; }
        }

        if (bestIdx >= 0) {
            Placement& chosen = placements[bestIdx];
            
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
                    sf::RectangleShape r(sf::Vector2f(CELL_SIZE-1, CELL_SIZE-1));
                    r.setPosition(BORDER + x * CELL_SIZE, BORDER + y * CELL_SIZE);
                    r.setFillColor(sf::Color(100,100,100));
                    window.draw(r);
                 }
            }

            for(const auto& p : placements) {
                drawTetriminoVis(window, current, p.rotation, p.x, p.y, BORDER, BORDER, CELL_SIZE, sf::Color(current.color.r, current.color.g, current.color.b, 30));
            }
            drawTetriminoVis(window, current, chosen.rotation, chosen.x, chosen.y, BORDER, BORDER, CELL_SIZE, current.color);
            
            sf::Text txt;
            txt.setFont(font);
            txt.setCharacterSize(20);
            txt.setString("Gen: " + std::to_string(generation) + "\nBest Fitness: " + std::to_string(bestFitness) + "\nLines: " + std::to_string(totalLines));
            txt.setPosition(BORDER + Board::WIDTH * CELL_SIZE + 20, BORDER);
            txt.setFillColor(sf::Color::White);
            window.draw(txt);
            
            window.display();
            sf::sleep(sf::milliseconds(50)); 

            board.applyPlacement(chosen, current);
            totalLines += chosen.clearedLines;
            current = Tetromino(bag.next());
        } else {
            break;
        }
    }
}

int main(){
    const int POP = 100, INPUTS = 4, OUTPUTS = 1;
    const std::string POP_STATE_FILE = "population_state.txt";
    neat::Population pop;
    std::ifstream state_in(POP_STATE_FILE);
    if(state_in.is_open()) { pop = neat::Population::deserialize(POP_STATE_FILE); } 
    else { pop = neat::Population(POP, INPUTS, OUTPUTS, (int)std::chrono::system_clock::now().time_since_epoch().count()); }

    const float CELL_SIZE = 20.f, BORDER = 20.f, UI_W = 200.f;
    sf::RenderWindow window(sf::VideoMode(Board::WIDTH*CELL_SIZE+UI_W+2*BORDER, Board::HEIGHT*CELL_SIZE+2*BORDER), "Tetris NEAT - Live Training");
    sf::Font font; font.loadFromFile("Arial.ttf");

    const int GENERATIONS = 500;
    
    for(int gen=0; gen<GENERATIONS && window.isOpen(); ++gen){
        
        std::vector<std::future<void>> futures;
        for(auto& genome : pop.genomes) {
            futures.push_back(std::async(std::launch::async, evaluate_genome_fitness, std::ref(genome), gen));
        }

        bool all_done = false;
        while(!all_done && window.isOpen()) {
            sf::Event ev;
            while(window.pollEvent(ev)){ if(ev.type==sf::Event::Closed) window.close(); }

            all_done = true;
            for(auto& fut : futures) {
                if(fut.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
                    all_done = false;
                    break;
                }
            }
            
            window.clear(sf::Color(30, 30, 40));
            sf::Text waitText("Calculating Fitness...", font, 30);
            waitText.setPosition(100, 250);
            window.draw(waitText);
            window.display();
        }
        if(!window.isOpen()) break;
        
        for(auto& fut : futures) { fut.get(); }

        std::sort(pop.genomes.begin(), pop.genomes.end(), [](const neat::Genome& a, const neat::Genome& b){ return a.fitness > b.fitness; });
        double bestFitness = pop.genomes[0].fitness;
        double sum = 0;
        for(const auto& g : pop.genomes) sum += g.fitness;
        std::cout << "Gen " << gen << " avg fitness " << (sum/POP) << " best " << bestFitness << "\n";

        visualizeGame(window, pop.genomes[0], font, gen, bestFitness);

        std::ofstream best_out("saved_genome.txt");
        std::stringstream ss;
        pop.genomes[0].serialize(ss);
        best_out << ss.str();
        best_out.close();

        pop.epoch(4);
        pop.serialize(POP_STATE_FILE);
    }
    return 0;
}