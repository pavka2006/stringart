#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include "image.h"
#include "models.h"
#include "algorithms.h"
#include "services.h"
#include "image_processor.h"

namespace fs = std::filesystem;

void ReportProgress(int current, int total, const char* msg) {
    int percent = (int)((current / (double)total) * 100);
    printf("[%03d%%] %s\n", percent, msg);
}

int main(int argc, char* argv[]) {
    std::cout << "=== String Art Generator v3.3 (C++) - Fast ===" << std::endl << std::endl;

    if (argc < 2) {
        std::cout << "Usage: StringArtGenerator [image_path] [output_dir]" << std::endl;
        std::cout << "Example: StringArtGenerator photo.png output" << std::endl;
        return 1;
    }

    std::string imagePath = argv[1];
    std::string outputDir = (argc > 2) ? argv[2] : "StringArtResults";

    if (!fs::exists(imagePath)) {
        std::cerr << "ERROR: Image not found: " << imagePath << std::endl;
        return 1;
    }

    try {
        std::cout << "\n========== STAGE 1: COARSE STRUCTURE ==========" << std::endl;

        GenerationParameters params1;
        params1.imageResolution = 360;
        params1.nailCount = 360;
        params1.maxIterations = 500;
        params1.lineAlpha = 0.05;
        params1.stage = 1;
        params1.exportJson = false;
        params1.exportPng = false;
        params1.inputImagePath = fs::absolute(imagePath).string();
        params1.outputDirectory = fs::absolute(outputDir).string();

        fs::create_directories(params1.outputDirectory);

        std::cout << "Input: " << params1.inputImagePath << std::endl;
        std::cout << "Output: " << params1.outputDirectory << std::endl;
        std::cout << "Resolution: " << params1.imageResolution << "x" << params1.imageResolution << std::endl;
        std::cout << "Nails: " << params1.nailCount << std::endl;
        std::cout << "Max iterations (Stage 1): " << params1.maxIterations << std::endl;
        std::cout << "Line Alpha (Stage 1): " << params1.lineAlpha << std::endl << std::endl;

        ReportProgress(1, 4, "Loading image...");

        ImageProcessor imgProc;

        Image targetMatrix = imgProc.LoadAndProcess(params1.inputImagePath, params1.imageResolution);

        ReportProgress(2, 4, "Generating nails...");

        Utils nailGen;
        auto nails = nailGen.GenerateNails(
            params1.nailCount,
            params1.imageResolution / 2.0,
            params1.imageResolution / 2.0,
            params1.imageResolution / 2.0 - 5
        );

        ReportProgress(3, 4, "Optimizing (Stage 1)...");

        LineCache cache;
        GreedyOptimizer optimizer(&cache);

        auto result1 = optimizer.Optimize(targetMatrix, nails, params1, ReportProgress);

        std::cout << "\n=== STAGE 1 RESULT ===" << std::endl;
        std::cout << "Lines: " << result1.lineSequence.size() << std::endl;
        std::cout << "MSE: " << std::fixed << std::setprecision(4) << result1.metrics.mse << std::endl;
        std::cout << "RMSE: " << result1.metrics.rmse << std::endl;

        std::cout << "\n========== STAGE 2: FINE TUNING ==========" << std::endl;

        GenerationParameters params2;
        params2.imageResolution = 360;
        params2.nailCount = 360;
        params2.maxIterations = 2000;
        params2.lineAlpha = 0.1;
        params2.stage = 2;
        params2.exportJson = true;
        params2.exportPng = true;
        params2.inputImagePath = params1.inputImagePath;
        params2.outputDirectory = params1.outputDirectory;

        std::cout << "Max iterations (Stage 2): " << params2.maxIterations << std::endl;
        std::cout << "Line Alpha (Stage 2): " << params2.lineAlpha << std::endl;
        std::cout << "Threshold: 0.005 (balanced)" << std::endl << std::endl;

        std::cout << "[050%] Optimizing (Stage 2)...\n";

        auto result2 = optimizer.Optimize(targetMatrix, nails, params2, ReportProgress);

        std::cout << "\n=== STAGE 2 RESULT ===" << std::endl;
        std::cout << "Lines: " << result2.lineSequence.size() << std::endl;
        std::cout << "MSE: " << result2.metrics.mse << std::endl;
        std::cout << "RMSE: " << result2.metrics.rmse << std::endl;

        std::cout << "\n========== EXPORTING ==========" << std::endl;
        std::cout << "[075%] Exporting...\n";

        Exporter exporter;

        if (params2.exportJson) {
            std::string jsonPath = params2.outputDirectory + "/result.json";
            exporter.ExportJson(result2, jsonPath);
            std::cout << "Saved: " << jsonPath << std::endl;
        }

        if (params2.exportPng) {
            std::string pngPath = params2.outputDirectory + "/result.png";
            exporter.ExportPng(result2, pngPath, params2.imageResolution);
            std::cout << "Saved: " << pngPath << std::endl;
        }

        std::cout << "\n=== FINAL RESULT ===" << std::endl;
        std::cout << "Stage 1 Lines: " << result1.lineSequence.size() << std::endl;
        std::cout << "Stage 2 Lines: " << result2.lineSequence.size() << std::endl;
        std::cout << "MSE: " << std::fixed << std::setprecision(4) << result2.metrics.mse << std::endl;
        std::cout << "RMSE: " << result2.metrics.rmse << std::endl;
        std::cout << "Coverage: " << std::setprecision(2) << result2.metrics.coveragePercent << "%" << std::endl;
        std::cout << "Total Time: " << result2.metrics.processingTimeMs << "ms" << std::endl;

        std::cout << "\n Complete!" << std::endl;

    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}