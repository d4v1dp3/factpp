#ifndef FACT_Splitting
#define FACT_Splitting

#include <map>
#include <vector>
#include <random>
#include <stdexcept>
#include <algorithm>

#include "Configuration.h"

namespace Tools
{
    class Splitting
    {
        std::uniform_real_distribution<double> distribution;
        std::mt19937_64 generator;
        uint64_t seed;

        std::vector<uint16_t> seq;
        std::vector<double> quant;

        size_t num;

        std::map<size_t, size_t> lut;

    public:
        static const po::options_description &options()
        {
            static po::options_description split("Splitting options");
            if (split.find_nothrow("seed", false))
                return split;

            split.add_options()
                ("split-sequence,S", vars<uint16_t>(), "Split data sequentially into several trees/files (e.g. 1, 1, 2)")
                ("split-quantile,Q", vars<double>(),   "Split data randomly into several trees/files (e.g. 0.5, 1)")
                ("seed", var<uint64_t>(std::mt19937_64::default_seed), "Seed value in case of random split")
                ;

            return split;
        }

        static const char *usage()
        {
            return
                "For several purposes, it might be convenient to split the output to several "
                "different root-treess. This can be done using the --split-sequence (-S) "
                "and the --split-quantile (-Q) options. If a split sequence is defined as "
                "-S 1 -S 2 -S 1 the events are split by 1:2:1 in this sequence order. If "
                "quantiles are given as -Q 0.5 -Q 0.6, the first tree will contain 50% of "
                "the second one 10% and the third one 40%. The corresponding seed value can "
                "be set with --seed.\n";
        }

        Splitting(Configuration &conf) : distribution(0, 1)
        {
            seq   = conf.Vec<uint16_t>("split-sequence");
            quant = conf.Vec<double>("split-quantile");

            if (!seq.empty() && !quant.empty())
                throw std::runtime_error("Only splitting by --split-sequence or --split-quantile is allowed.");

            num = seq.size()+quant.size()==0 ? 0 : std::max(seq.size(), quant.size()+1);

            for (size_t i=0; i<seq.size(); i++)
            {
                const size_t sz = lut.size();
                for (size_t j=0; j<seq[i]; j++)
                    lut.emplace(j+sz, i);
            }

            for (size_t i=0; i<quant.size(); i++)
                if (quant[i]<0 || quant[i]>=1)
                    throw std::runtime_error("Splitting quantiles must be in the range [0;1)");

            for (size_t i=1; i<quant.size(); i++)
            {
                if (quant[i]<=quant[i-1])
                    throw std::runtime_error("Splitting quantiles must be in increasing order.");
            }

            seed = conf.Get<uint64_t>("seed");
            generator.seed(seed);

            //auto rndm = std::bind(distribution, generator);
            //(bind(&StateMachineFTM::ResetConfig, this))
        }

        size_t index(const size_t &count) /*const*/
        {
            size_t index = 0;
            if (!lut.empty())
                index = lut.find(count % lut.size())->second;

            if (quant.empty())
                return index;

            const double rndm = distribution(generator);
            for (; rndm>=quant[index]; index++)
                if (index==quant.size())
                    return index;

            return index;
        }

        void print()
        {
            if (!num)
                return;

            std::cout << "Splitting configured " << (seq.empty()?"randomly":"in sequence") << " into " << num << " branches.";
            if (!quant.empty())
                std::cout << "\nSeed value configured as " << seed << ".";
            std::cout << std::endl;
        }

        const size_t &size() const
        {
            return num;
        }

        const bool empty() const
        {
            return num==0;
        }
    };
};
#endif
