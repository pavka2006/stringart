#pragma once
#include <string>
#include <vector>
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

class QualityMetrics {
private:
    double mse;
    double rmse;
    double coveragePercent;
    int totalLines;
    long processingTimeMs;

public:
    QualityMetrics()
        : mse(0.0), rmse(0.0), coveragePercent(0.0), totalLines(0), processingTimeMs(0) {}

    double getMse() const { return mse; }
    double getRmse() const { return rmse; }
    double getCoveragePercent() const { return coveragePercent; }
    int getTotalLines() const { return totalLines; }
    long getProcessingTimeMs() const { return processingTimeMs; }

    void setMse(double value) { mse = value; }
    void setRmse(double value) { rmse = value; }
    void setCoveragePercent(double value) { coveragePercent = value; }
    void setTotalLines(int value) { totalLines = value; }
    void setProcessingTimeMs(long value) { processingTimeMs = value; }
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
    Image renderedImage;
    QualityMetrics metrics;
    std::vector<Nail> nails;
};