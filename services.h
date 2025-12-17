#pragma once

#include "image.h"
#include "models.h"
#include "algorithms.h"
#include <vector>
#include <map>
#include <deque>
#include <chrono>

class Utils {//как и хотели, теперь тут утилиты всякие
public:
    std::vector<Nail> GenerateNails(int count, double centerX, double centerY, double radius) {
        std::vector<Nail> nails;
        double angleStep = 360.0 / count;

        for (int i = 0; i < count; i++) {
            double angle = i * angleStep;
            double radians = angle * std::acos(-1.0) / 180.0;
            double x = centerX + radius * std::cos(radians);
            double y = centerY + radius * std::sin(radians);
            nails.emplace_back(i, x, y, angle);
        }

        return nails;
    }
};

class LineCache {
private:
    std::map<std::pair<int, int>, std::vector<int>> cache;
    std::vector<Nail> nails;

public:
    void Precompute(int nailCount, int width, int height) {//тут войд потому что просто кеширует и ничего не возвращает
        cache.clear();
        Utils gen;
        nails = gen.GenerateNails(nailCount, width / 2.0, height / 2.0, width / 2.0 - 5);

        for (int i = 0; i < nailCount; i++) {
            for (int j = i + 1; j < nailCount; j++) {
                auto pixels = Algorithms::BresenhamLine(
                    (int)nails[i].x, (int)nails[i].y,
                    (int)nails[j].x, (int)nails[j].y,
                    width, height
                );
                cache[{i, j}] = pixels;
            }
        }
    }

    const std::vector<int>& GetLine(int from, int to) {
        if (from > to) std::swap(from, to);
        auto it = cache.find({from, to});
        if (it != cache.end()) return it->second;
        static const std::vector<int> empty;
        return empty;
    }
};

class GreedyOptimizer {
private:
    LineCache* cache;
    std::deque<std::pair<int, double>> recentImprovements;
    void ApplyLineWithAlpha(Image& intensity, int from, int to, double lineAlpha) {
        const auto& pixels = cache->GetLine(from, to);

        for (int idx : pixels) {
            int y = idx / intensity.getWidth();
            int x = idx % intensity.getWidth();

            if (x >= 0 && x < intensity.getWidth() && y >= 0 && y < intensity.getHeight()) {
                intensity.at(x, y) = intensity.at(x, y) * (1.0 - lineAlpha) + lineAlpha;
            }
        }
    }

public:
    GreedyOptimizer(LineCache* cache) : cache(cache) {}
    GenerationResult Optimize(const Image& target,
                             const std::vector<Nail>& nails,
                             const GenerationParameters& params,
                             void (*progress)(int, int, const char*) = nullptr) {
        GenerationResult result;
        result.nails = nails;

        int size = target.getWidth();

        Image intensity(size, size);
        intensity.fill(0.0);

        int current = 0;
        auto startTime = std::chrono::high_resolution_clock::now();

        cache->Precompute(nails.size(), size, size);

        int minGap = (params.stage == 1) ? 16 : 8;
        double lineAlpha = (params.stage == 1) ? 0.05 : 0.1;
        int maxIterations = params.maxIterations;

        for (int iter = 0; iter < maxIterations; iter++) {
            int best = -1;
            double bestImpr = -1.0;

            Image bestIntensity(size, size);
            bestIntensity.fill(0.0);

            for (int cand = 0; cand < (int)nails.size(); cand++) {
                if (cand == current) continue;

                int d = std::abs(cand - current);
                d = std::min(d, (int)nails.size() - d);

                if (d < minGap) continue;

                Image temp = intensity;

                ApplyLineWithAlpha(temp, current, cand, lineAlpha);

                double impr = Algorithms::CalculateImprovement(target, intensity, temp);

                if (impr > bestImpr) {
                    bestImpr = impr;
                    best = cand;
                    bestIntensity = temp;
                }
            }

            if (best == -1 || bestImpr <= 0.005) break;

            intensity = bestIntensity;
            result.lineSequence.emplace_back(current, best, result.lineSequence.size());
            current = best;

            recentImprovements.push_back({iter, bestImpr});
            if (recentImprovements.size() > 100) recentImprovements.pop_front();

            if (progress && iter % 100 == 0) {
                progress(iter, maxIterations, "Optimizing...");
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        result.renderedImage = intensity;
        result.metrics.mse = Algorithms::CalculateMSE(target, intensity);
        result.metrics.rmse = Algorithms::CalculateRMSE(target, intensity);
        result.metrics.coveragePercent = Algorithms::CalculateCoveragePercent(intensity);
        result.metrics.totalLines = result.lineSequence.size();
        result.metrics.processingTimeMs = duration.count();

        return result;
    }
};