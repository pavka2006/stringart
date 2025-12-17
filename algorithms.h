#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include "image.h"
#include "models.h"

namespace Algorithms {

std::vector<int> BresenhamLine(int x0, int y0, int x1, int y1, int width, int height) {
    std::vector<int> pixels;
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int x = x0, y = y0;

    while (true) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            pixels.push_back(y * width + x);
        }

        if (x == x1 && y == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }

    return pixels;
}

double CalculateMSE(const Image& target, const Image& rendered) {
    if (target.getWidth() != rendered.getWidth() || target.getHeight() != rendered.getHeight()) {
        return 0.0;
    }

    int size = target.getWidth();
    if (size == 0) return 0.0;

    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            double predicted = 255.0 - (rendered.at(j, i) * 255.0);
            double diff = target.at(j, i) - predicted;
            sum += diff * diff;
        }
    }

    return sum / (size * size);
}

double CalculateRMSE(const Image& target, const Image& rendered) {
    return std::sqrt(CalculateMSE(target, rendered));
}

double CalculateImprovement(const Image& target, const Image& before, const Image& after) {
    double mseBefore = CalculateMSE(target, before);
    double mseAfter = CalculateMSE(target, after);
    return mseBefore - mseAfter;
}

double CalculateCoveragePercent(const Image& rendered) {
    int width = rendered.getWidth();
    int height = rendered.getHeight();
    int covered = 0;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (rendered.at(j, i) > 0.01) covered++;
        }
    }

    return (covered / (double)(width * height)) * 100.0;
}

inline double ToGrayDouble(int r, int g, int b) {
    return 0.299 * r + 0.587 * g + 0.114 * b;
}

}