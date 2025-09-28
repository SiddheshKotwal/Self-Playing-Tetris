#pragma once
#include <vector>
#include <map>
#include <random>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>

namespace neat
{

    using rng_t = std::mt19937;

    inline double sigmoid(double x) { return 1.0 / (1.0 + std::exp(-4.9 * x)); }

    struct NodeGene
    {
        int id;
        int type; // 0=input,1:hidden,2=output,3=bias
    };

    struct ConnGene
    {
        int in, out;
        double weight;
        bool enabled;
        int innov;
    };

    struct Genome
    {
        std::vector<NodeGene> nodes;
        std::vector<ConnGene> conns;
        double fitness = 0.0;

        double evaluate(const std::vector<double> &inputs) const
        {
            std::map<int, double> value;
            for (auto &n : nodes)
            {
                if (n.type == 0)
                    value[n.id] = 0.0;
                if (n.type == 3)
                    value[n.id] = 1.0; // bias
            }
            int inIdx = 0;
            for (auto &n : nodes)
            {
                if (n.type == 0)
                {
                    if (inIdx < (int)inputs.size())
                        value[n.id] = inputs[inIdx++];
                    else
                        value[n.id] = 0.0;
                }
            }
            for (int pass = 0; pass < 3; ++pass)
            {
                for (const auto &c : conns)
                {
                    if (!c.enabled)
                        continue;
                    double inV = value.count(c.in) ? value.at(c.in) : 0.0;
                    double outV = value.count(c.out) ? value.at(c.out) : 0.0;
                    outV += inV * c.weight;
                    value[c.out] = outV;
                }
                for (auto &n : nodes)
                {
                    if (n.type == 1 || n.type == 2)
                    {
                        value[n.id] = sigmoid(value[n.id]);
                    }
                }
            }
            double best = -1e9;
            for (auto &n : nodes)
            {
                if (n.type == 2)
                {
                    best = std::max(best, value[n.id]);
                }
            }
            return best;
        }

        void mutateWeights(rng_t &rng, double perturbProb = 0.9, double step = 0.5)
        {
            std::uniform_real_distribution<double> unif(0, 1);
            std::normal_distribution<double> norm(0, step);
            for (auto &c : conns)
            {
                if (unif(rng) < perturbProb)
                {
                    c.weight += norm(rng);
                }
                else
                {
                    c.weight = std::uniform_real_distribution<double>(-1, 1)(rng);
                }
            }
        }

        void addConnection(rng_t &rng, int &globalInnov)
        {
            if (nodes.size() < 2)
                return;
            std::uniform_int_distribution<int> pick(0, nodes.size() - 1);
            int a = pick(rng), b = pick(rng);
            int in = nodes[a].id, out = nodes[b].id;
            if (nodes[a].type > nodes[b].type)
            {
                std::swap(a, b);
                in = nodes[a].id;
                out = nodes[b].id;
            }
            if (nodes[a].type == 2 || nodes[b].type == 0 || nodes[b].type == 3)
                return;
            for (auto &c : conns)
                if (c.in == in && c.out == out)
                    return;
            ConnGene cg;
            cg.in = in;
            cg.out = out;
            cg.weight = std::uniform_real_distribution<double>(-1, 1)(rng);
            cg.enabled = true;
            cg.innov = globalInnov++;
            conns.push_back(cg);
        }

        void addNode(rng_t &rng, int &globalInnov, int &nextNodeId)
        {
            if (conns.empty())
                return;
            std::uniform_int_distribution<int> pick(0, conns.size() - 1);
            int idx = pick(rng);
            if (!conns[idx].enabled)
                return;
            conns[idx].enabled = false;
            NodeGene ng;
            ng.id = nextNodeId++;
            ng.type = 1;
            nodes.push_back(ng);
            ConnGene c1;
            c1.in = conns[idx].in;
            c1.out = ng.id;
            c1.weight = 1.0;
            c1.enabled = true;
            c1.innov = globalInnov++;
            ConnGene c2;
            c2.in = ng.id;
            c2.out = conns[idx].out;
            c2.weight = conns[idx].weight;
            c2.enabled = true;
            c2.innov = globalInnov++;
            conns.push_back(c1);
            conns.push_back(c2);
        }

        static Genome crossover(const Genome &a, const Genome &b, rng_t &rng)
        {
            Genome child;
            std::map<int, NodeGene> nodeMap;
            for (auto &n : a.nodes)
                nodeMap[n.id] = n;
            for (auto &n : b.nodes)
                if (nodeMap.find(n.id) == nodeMap.end())
                    nodeMap[n.id] = n;
            for (auto &kv : nodeMap)
                child.nodes.push_back(kv.second);

            std::map<int, const ConnGene *> innovA;
            for (const auto &c : a.conns)
                innovA[c.innov] = &c;
            std::map<int, const ConnGene *> innovB;
            for (const auto &c : b.conns)
                innovB[c.innov] = &c;

            for (auto const &[innov, geneA] : innovA)
            {
                auto itB = innovB.find(innov);
                if (itB != innovB.end())
                { // Matching gene
                    child.conns.push_back(std::uniform_real_distribution<>(0, 1)(rng) < 0.5 ? *geneA : *(itB->second));
                }
                else
                { // Disjoint/Excess from A
                    child.conns.push_back(*geneA);
                }
            }
            return child;
        }

        void serialize(std::ostream &os) const
        {
            os << nodes.size() << "\n";
            for (auto &n : nodes)
                os << n.id << " " << n.type << "\n";
            os << conns.size() << "\n";
            for (auto &c : conns)
                os << c.innov << " " << c.in << " " << c.out << " " << c.weight << " " << c.enabled << "\n";
        }

        void deserialize(std::istream &is)
        {
            nodes.clear();
            conns.clear();
            int nNodes;
            is >> nNodes;
            for (int i = 0; i < nNodes; ++i)
            {
                NodeGene ng;
                is >> ng.id >> ng.type;
                nodes.push_back(ng);
            }
            int nConns;
            is >> nConns;
            for (int i = 0; i < nConns; ++i)
            {
                ConnGene c;
                is >> c.innov >> c.in >> c.out >> c.weight >> c.enabled;
                conns.push_back(c);
            }
        }
    };

    struct Population
    {
        std::vector<Genome> genomes;
        rng_t rng;
        int globalInnov = 1;
        int nextNodeId = 1000;

        Population() = default;

        Population(int populationSize, int numInputs, int numOutputs, int seed = 42)
        {
            rng.seed(seed);
            genomes.resize(populationSize);
            for (auto &g : genomes)
            {
                g.nodes.clear();
                int next = 0;
                for (int i = 0; i < numInputs; ++i)
                    g.nodes.push_back({next++, 0});
                g.nodes.push_back({next++, 3});
                for (int o = 0; o < numOutputs; ++o)
                    g.nodes.push_back({next++, 2});
                nextNodeId = std::max(nextNodeId, next);
                for (auto &inN : g.nodes)
                {
                    if (inN.type == 0 || inN.type == 3)
                    {
                        for (auto &outN : g.nodes)
                            if (outN.type == 2)
                            {
                                ConnGene c;
                                c.in = inN.id;
                                c.out = outN.id;
                                c.weight = std::uniform_real_distribution<double>(-1, 1)(rng);
                                c.enabled = true;
                                c.innov = globalInnov++;
                                g.conns.push_back(c);
                            }
                    }
                }
            }
        }

        void epoch(int elites = 2)
        {
            std::sort(genomes.begin(), genomes.end(), [](const Genome &a, const Genome &b)
                      { return a.fitness > b.fitness; });
            std::vector<Genome> next;
            for (int i = 0; i < elites && i < (int)genomes.size(); ++i)
                next.push_back(genomes[i]);
            while ((int)next.size() < (int)genomes.size())
            {
                int a = std::min((int)genomes.size() - 1, (int)(std::pow(std::uniform_real_distribution<double>(0, 1)(rng), 2) * genomes.size()));
                int b = std::min((int)genomes.size() - 1, (int)(std::pow(std::uniform_real_distribution<double>(0, 1)(rng), 2) * genomes.size()));
                const Genome &parentA = genomes[a].fitness > genomes[b].fitness ? genomes[a] : genomes[b];
                const Genome &parentB = genomes[a].fitness > genomes[b].fitness ? genomes[b] : genomes[a];
                Genome child = Genome::crossover(parentA, parentB, rng);
                child.mutateWeights(rng);
                if (std::uniform_real_distribution<double>(0, 1)(rng) < 0.05)
                    child.addNode(rng, globalInnov, nextNodeId);
                if (std::uniform_real_distribution<double>(0, 1)(rng) < 0.2)
                    child.addConnection(rng, globalInnov);
                next.push_back(child);
            }
            genomes.swap(next);
        }

        void serialize(const std::string &filename) const
        {
            std::ofstream os(filename);
            os << globalInnov << " " << nextNodeId << "\n";
            os << genomes.size() << "\n";
            for (const auto &g : genomes)
            {
                g.serialize(os);
            }
        }

        static Population deserialize(const std::string &filename)
        {
            Population pop;
            std::ifstream is(filename);
            is >> pop.globalInnov >> pop.nextNodeId;
            int nGenomes;
            is >> nGenomes;
            pop.genomes.resize(nGenomes);
            for (int i = 0; i < nGenomes; ++i)
            {
                pop.genomes[i].deserialize(is);
            }
            return pop;
        }
    };
}