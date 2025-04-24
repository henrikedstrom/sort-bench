// sort_bench.cpp: Compares the performance of a custom 3-pass radix sort against std::sort.
// Generates test inputs, runs both algorithms over multiple trials, and reports throughput in M elements/sec.
//

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
// Configuration

struct Config {
  static constexpr uint32_t NumElements = 1 * 1024 * 1024;
  static constexpr uint32_t NumTrials = 10;
  static constexpr bool CheckCorrectness = true;
  static constexpr bool MostlySorted = false;
};

// ------------------------------------------------------------------------------------------------
// Utility functions

void generateInputElements(std::vector<std::vector<float>>& inputs) {
  std::mt19937 rng(1234);
  std::uniform_real_distribution<float> dist(-16.0f, 16.0f);

  inputs.resize(Config::NumTrials);

  if (Config::MostlySorted) {
    uint32_t offsetRange =
        uint32_t(Config::NumElements * 0.15f);  // 15% offset
    uint32_t numToDisplace =
        uint32_t(Config::NumElements * 0.10f);  // 10% of total

    for (uint32_t t = 0; t < Config::NumTrials; ++t) {
      inputs[t].resize(Config::NumElements);

      // Start with a perfectly sorted array
      std::vector<std::pair<float, uint32_t>> sortedPairs(Config::NumElements);
      for (uint32_t i = 0; i < Config::NumElements; ++i) {
        sortedPairs[i] = {dist(rng), i};
      }
      std::sort(sortedPairs.begin(), sortedPairs.end());

      // Copy into input set
      for (uint32_t i = 0; i < Config::NumElements; ++i) {
        inputs[t][i] = sortedPairs[i].first;
      }

      // Displace 'numToDisplace' of them slightly
      for (uint32_t j = 0; j < numToDisplace; ++j) {
        uint32_t idx = rng() % Config::NumElements;

        int32_t offset = int32_t(rng() % (2 * offsetRange + 1)) -
                         int32_t(offsetRange);  // [-range, +range]
        int32_t swapWith = int32_t(idx) + offset;

        // Clamp to valid range
        if (swapWith < 0) swapWith = 0;
        if (swapWith >= int32_t(Config::NumElements))
          swapWith = Config::NumElements - 1;

        // Swap in both input sets
        std::swap(inputs[t][idx], inputs[t][swapWith]);
      }
    }
  } else {
    for (uint32_t t = 0; t < Config::NumTrials; ++t) {
      inputs[t].resize(Config::NumElements);
      for (uint32_t i = 0; i < Config::NumElements; ++i) {
        float val = dist(rng);
        inputs[t][i] = val;
      }
    }
  }
}

// ------------------------------------------------------------------------------------------------
// Main function
int main()
{
  //--------------------------------------
  // RadixSort

  // Input and output arrays
  std::vector<std::vector<float>> trialInputsRadix;
  std::vector<float> radixOut(Config::NumElements);
  generateInputElements(trialInputsRadix);

  // Start benchmark
  auto startRadix = std::chrono::high_resolution_clock::now();

  for (uint32_t t = 0; t < Config::NumTrials; ++t) {
    RadixSort11(trialInputsRadix[t].data(), radixOut.data(),
                Config::NumElements);
  }

  // End benchmark
  auto endRadix = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> durationRadix = endRadix - startRadix;
  double radix_eps =
      (Config::NumElements * Config::NumTrials) / durationRadix.count();

  // Optionally, check correctness
  if (Config::CheckCorrectness) {
    for (uint32_t i = 1; i < Config::NumElements; i++) {
      if (radixOut[i - 1] > radixOut[i]) {
        std::cout << "Radix sort wrong at " << i << "\n";
        break;
      }
    }
  }

  //--------------------------------------
  // std::sort

  // Generate input
  std::vector<std::vector<float>> trialInputsStd;
  generateInputElements(trialInputsStd);

  // Start benchmark
  auto startStd = std::chrono::high_resolution_clock::now();

  for (uint32_t t = 0; t < Config::NumTrials; ++t) {
    std::sort(trialInputsStd[t].begin(), trialInputsStd[t].end());
  }

  // End benchmark
  auto endStd = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> durationStd = endStd - startStd;
  double std_eps =
      (Config::NumElements * Config::NumTrials) / durationStd.count();

  // Optionally, check correctness
  if (Config::CheckCorrectness) {
    for (uint32_t i = 1; i < Config::NumElements; i++) {
      if (trialInputsStd[Config::NumTrials - 1][i - 1] >
          trialInputsStd[Config::NumTrials - 1][i]) {
        std::cout << "std::sort wrong at " << i << "\n";
        break;
      }
    }
  }

  //--------------------------------------
  // Print results

  double speedup = std_eps > 0.0 ? radix_eps / std_eps : 0.0;

  std::cout << std::fixed << std::setprecision(2);
  std::cout << std::fixed << std::setprecision(2);
  std::cout << "[RadixSort11] " << Config::NumElements
            << " elements: " << (radix_eps / 1'000'000.0)
            << " M elements/sec\n";
  std::cout << "[std::sort]   " << Config::NumElements
            << " elements: " << (std_eps / 1'000'000.0) << " M elements/sec\n";

  std::cout << "RadixSort11 is " << speedup << "x faster than std::sort\n";

  return 0;
}
