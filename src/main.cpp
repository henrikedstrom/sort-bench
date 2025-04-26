// sort_bench.cpp
// Benchmarks std::sort vs RadixSort11 over a range of input sizes,
// for both random and mostly-sorted inputs.

// Standard Library Headers
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

// Project Headers
#include "radix.h"

// ------------------------------------------------------------------------------------------------
// Config parameters

static constexpr uint32_t kMaxTotal = 16 * 1024 * 1024; // cap N * trials to 16M
static constexpr uint32_t kMaxTrials = 128;
static constexpr bool kCheckCorrect = true; // Verify sorting order

// ------------------------------------------------------------------------------------------------
// Utility functions

// generate 'trials' independent vectors of length 'N'
// if 'mostlySorted' is true, start with a sorted list and then displace some (default 10%) of the elements
void generateInputs(uint32_t trials, uint32_t N, bool mostlySorted, std::vector<std::vector<float>> &out)
{
    std::mt19937 rng(1234);
    std::uniform_real_distribution<float> dist(-16.0f, 16.0f);
    out.assign(trials, std::vector<float>(N));

    if (mostlySorted)
    {
        uint32_t offsetRange = uint32_t(N * 0.15f); // +/- 15% of N
        uint32_t displace = uint32_t(N * 0.10f);    // displace 10% of the elements
        for (uint32_t t = 0; t < trials; ++t)
        {
            auto &v = out[t];
            for (uint32_t i = 0; i < N; ++i)
            {
                v[i] = dist(rng);
            }

            // start with a sorted list
            std::sort(v.begin(), v.end());

            // displace
            for (uint32_t j = 0; j < displace; ++j)
            {
                uint32_t i = rng() % N;
                int32_t off = int32_t(rng() % (2 * offsetRange + 1)) - int32_t(offsetRange);
                uint32_t k = uint32_t(std::clamp<int32_t>(i + off, 0, int32_t(N - 1)));
                std::swap(v[i], v[k]);
            }
        }
    }
    else
    {
        for (uint32_t t = 0; t < trials; ++t)
        {
            auto &v = out[t];
            for (float &x : v)
            {
                x = dist(rng);
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Main function

int main()
{
    // Run two passes/scenarios: random input and mostly-sorted input
    struct Scenario
    {
        const char *label;
        bool mostlySorted;
    };
    const Scenario scenarios[2] = {{"Random Input", false}, {"Mostly-Sorted Input", true}};

    std::vector<std::vector<float>> inputsStd, inputsRadix;
    inputsStd.reserve(kMaxTrials);
    inputsRadix.reserve(kMaxTrials);

    // For each scenario, print a table:
    for (auto &s : scenarios)
    {
        // Print header
        std::cout << "\n=== " << s.label << " (million elements/sec) ===\n";

        // single‐row header, widths tuned to fit content
        std::cout << std::fixed << std::setprecision(2) << std::setw(12) << "Elements" << std::setw(16) << "std::sort"
                  << std::setw(16) << "Radix" << std::setw(12) << "Speedup"
                  << "\n";

        // sizes 2^1 .. 2^24
        for (int e = 1; e <= 24; ++e)
        {
            uint32_t N = 1u << e;
            // cap trials to keep the time reasonable
            uint32_t trials = std::min(kMaxTrials, std::max(1u, kMaxTotal / N));

            // generate inputs for this scenario
            generateInputs(trials, N, s.mostlySorted, inputsStd);
            generateInputs(trials, N, s.mostlySorted, inputsRadix);

            // output buffer for radix
            std::vector<float> radixOut(N);

            // --- std::sort
            auto t0 = std::chrono::high_resolution_clock::now();
            for (uint32_t t = 0; t < trials; ++t)
            {
                std::sort(inputsStd[t].begin(), inputsStd[t].end());
            }
            auto t1 = std::chrono::high_resolution_clock::now();

            double durStd = std::chrono::duration<double>(t1 - t0).count();
            double epsStd = double(N) * trials / durStd / 1e6;

            if (kCheckCorrect)
            {
                if (!std::is_sorted(inputsStd.back().begin(), inputsStd.back().end()))
                    std::cerr << "std::sort failed at N=" << N << "\n";
            }

            // --- RadixSort11
            t0 = std::chrono::high_resolution_clock::now();
            for (uint32_t t = 0; t < trials; ++t)
            {
                RadixSort11(inputsRadix[t].data(), radixOut.data(), N);
            }
            t1 = std::chrono::high_resolution_clock::now();

            double durRadix = std::chrono::duration<double>(t1 - t0).count();
            double epsRadix = double(N) * trials / durRadix / 1e6;

            if (kCheckCorrect)
            {
                if (!std::is_sorted(radixOut.begin(), radixOut.end()))
                    std::cerr << "RadixSort11 failed at N=" << N << "\n";
            }

            double speedup = epsRadix / epsStd;

            // print row
            std::cout << std::setw(12) << N << std::setw(16) << epsStd << std::setw(16) << epsRadix << std::setw(11)
                      << speedup << "x\n";
        }
    }

    return 0;
}
