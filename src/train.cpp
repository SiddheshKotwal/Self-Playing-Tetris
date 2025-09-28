#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <thread>
#include <future>
#include <vector>
#include <algorithm>
#include "game/Board.h"
#include "game/Tetrimino.h"
#include "neat/NEAT.h"

// --- CONFIGURATION FOR METRICS ---
const bool PARALLEL_EXECUTION = true;

// 549ms -> one generation -> with parallelisation
// 2669ms -> one generation -> without parallelisation

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
    const int NUM_GAMES_PER_EVAL = 3;
    int fitness = 0;
    for(int s=0; s<NUM_GAMES_PER_EVAL; ++s){
        fitness += linesClearedInGame(g, gen*10000 + g.nodes[0].id * 10 + s);
    }
    g.fitness = fitness;
}

int main(){
    const int POP = 100;
    const int INPUTS = 4;
    const int OUTPUTS = 1;
    const std::string POP_STATE_FILE = "population_state.txt";
    
    neat::Population pop;
    std::ifstream state_in(POP_STATE_FILE);
    if(state_in.is_open()) {
        std::cout << "Resuming training from " << POP_STATE_FILE << std::endl;
        pop = neat::Population::deserialize(POP_STATE_FILE);
    } else {
        std::cout << "Starting new training session." << std::endl;
        pop = neat::Population(POP, INPUTS, OUTPUTS, (int)std::chrono::system_clock::now().time_since_epoch().count());
    }
    
    // Setup for logging
    std::ofstream log_file("training_log.csv");
    log_file << "Generation,AverageFitness,BestFitness,BestFitnessAvgPerGame\n";

    const int GENERATIONS = 50;

    for(int gen=0; gen<GENERATIONS; ++gen){
        auto start_time = std::chrono::high_resolution_clock::now();

        if (PARALLEL_EXECUTION) {
            std::vector<std::future<void>> futures;
            for(auto& genome : pop.genomes) {
                futures.push_back(std::async(std::launch::async, evaluate_genome_fitness, std::ref(genome), gen));
            }
            for(auto& fut : futures) { fut.get(); }
        } else { // Serial execution for benchmarking
            for(auto& genome : pop.genomes) {
                evaluate_genome_fitness(genome, gen);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        double sum = 0; double best_fitness = -1;
        for(const auto& g : pop.genomes){ sum += g.fitness; best_fitness = std::max(best_fitness, g.fitness); }
        
        double avg_fitness = sum / POP;
        double best_fitness_avg_per_game = best_fitness / 3.0; // New, more intuitive metric

        std::cout << "Gen " << gen << " | Time: " << duration.count() << "ms | Avg Fitness: " << avg_fitness << " | Best Fitness: " << best_fitness << " (Avg/Game: " << best_fitness_avg_per_game << ")" << std::endl;
        
        // Write data to log file, including the new metric
        log_file << gen << "," << avg_fitness << "," << best_fitness << "," << best_fitness_avg_per_game << "\n";
        
        std::sort(pop.genomes.begin(), pop.genomes.end(), [](const neat::Genome& a, const neat::Genome& b){ return a.fitness > b.fitness; });
        
        std::ofstream best_out("saved_genome.txt");
        std::stringstream ss;
        pop.genomes[0].serialize(ss);
        best_out << ss.str();
        best_out.close();

        pop.epoch(4);
        pop.serialize(POP_STATE_FILE);
    }
    
    log_file.close();
    std::cout << "Training finished. Log saved to training_log.csv" << std::endl;
    return 0;
}