#pragma once

#include <vector>
#include <string>
#include "image.h"

struct Nail {
    int id;
    double x, y;
    double angle;

    Nail(int id = 0, double x = 0, double y = 0, double angle = 0)
        : id(id), x(x), y(y), angle(angle) {}
};

struct LineConnection {
    int fromNailId;
    int toNailId;
    int position;

    LineConnection(int from, int to, int pos)
        : fromNailId(from), toNailId(to), position(pos) {}
};

struct QualityMetrics {
    double mse = 0.0;
    double rmse = 0.0;
    double coveragePercent = 0.0;
    int totalLines = 0;
    long processingTimeMs = 0;
};

struct GenerationParameters {
    std::string inputImagePath;
    std::string outputDirectory;
    int imageResolution = 360;
    int nailCount = 360;
    int maxIterations = 1000;
    bool exportJson = true;
    bool exportPng = true;
    double lineAlpha = 0.1;
    int stage = 1;
};

struct GenerationResult {
    std::vector<LineConnection> lineSequence;
    Image renderedImage;  // From image.h
    QualityMetrics metrics;
    std::vector<Nail> nails;
};