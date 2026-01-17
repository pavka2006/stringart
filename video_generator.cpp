#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdlib>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace fs = std::filesystem;

struct Nail {
    int id;
    double x, y;
    double angle;
    
    Nail(int id = 0, double x = 0, double y = 0, double angle = 0)
        : id(id), x(x), y(y), angle(angle) {}
};

struct GenerationConfig {
    int imageResolution = 360;
    int nailCount = 360;
    std::string jsonPath;
    std::string outputDir = "video_frames";
    int frameSkip = 1;
    double lineAlpha = 0.1;
    int fps = 30;
    bool deleteFramesAfter = true;
    std::string ffmpegPath = "ffmpeg";
};

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

class SimpleJsonParser {
private:
    std::vector<int> ParseArray(const std::string& arrayStr) {
        std::vector<int> result;
        std::string cleaned = arrayStr;
        
        if (cleaned.front() == '[') cleaned = cleaned.substr(1);
        if (cleaned.back() == ']') cleaned = cleaned.substr(0, cleaned.length() - 1);
        
        std::stringstream ss(cleaned);
        std::string token;
        while (std::getline(ss, token, ',')) {
            result.push_back(std::stoi(token));
        }
        return result;
    }
    
    int ParseInt(const std::string& str) {
        return std::stoi(str);
    }
    
public:
    int nailCount = 0;
    int totalLines = 0;
    std::vector<int> threadSequence;
    
    bool Load(const std::string& path) {
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                std::cerr << "ERROR: Cannot open JSON file: " << path << std::endl;
                return false;
            }
            
            std::string line;
            std::string content;
            while (std::getline(file, line)) {
                content += line;
            }
            file.close();
            
            size_t nailPos = content.find("\"nail_count\"");
            if (nailPos != std::string::npos) {
                size_t colonPos = content.find(":", nailPos);
                size_t commaPos = content.find(",", colonPos);
                std::string nailStr = content.substr(colonPos + 1, commaPos - colonPos - 1);
                nailCount = ParseInt(nailStr);
            }
            
            size_t linesPos = content.find("\"total_lines\"");
            if (linesPos != std::string::npos) {
                size_t colonPos = content.find(":", linesPos);
                size_t commaPos = content.find(",", colonPos);
                std::string linesStr = content.substr(colonPos + 1, commaPos - colonPos - 1);
                totalLines = ParseInt(linesStr);
            }
            
            size_t seqPos = content.find("\"thread_sequence\"");
            if (seqPos != std::string::npos) {
                size_t arrayStart = content.find("[", seqPos);
                size_t arrayEnd = content.rfind("]");
                if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
                    std::string arrayStr = content.substr(arrayStart, arrayEnd - arrayStart + 1);
                    threadSequence = ParseArray(arrayStr);
                }
            }
            
            std::cout << "✓ Loaded JSON:" << std::endl;
            std::cout << "  - Nails: " << nailCount << std::endl;
            std::cout << "  - Total lines: " << totalLines << std::endl;
            std::cout << "  - Thread sequence length: " << threadSequence.size() << std::endl;
            
            return true;
        } catch (const std::exception& ex) {
            std::cerr << "ERROR: JSON parse failed: " << ex.what() << std::endl;
            return false;
        }
    }
};

class VideoFrameGenerator {
private:
    std::vector<Nail> nails;
    std::vector<double> intensity;
    int resolution;
    double lineAlpha;
    std::string outputDir;
    
public:
    VideoFrameGenerator(int resolution, double lineAlpha, const std::string& outputDir)
        : resolution(resolution), lineAlpha(lineAlpha), outputDir(outputDir) {
        intensity.resize(resolution * resolution, 0.0);
    }
    
    void SetNails(const std::vector<Nail>& nails_) {
        nails = nails_;
    }
    
    void ApplyLineWithAlpha(int fromNail, int toNail) {
        auto pixels = BresenhamLine(
            (int)nails[fromNail].x, (int)nails[fromNail].y,
            (int)nails[toNail].x, (int)nails[toNail].y,
            resolution, resolution
        );
        
        for (int idx : pixels) {
            int y = idx / resolution;
            int x = idx % resolution;
            if (x >= 0 && x < resolution && y >= 0 && y < resolution) {
                intensity[idx] = intensity[idx] * (1.0 - lineAlpha) + lineAlpha;
            }
        }
    }
    
    void SaveFrame(int frameNumber) {
        std::vector<unsigned char> imgData(resolution * resolution * 3);
        
        for (int y = 0; y < resolution; y++) {
            for (int x = 0; x < resolution; x++) {
                double intens = intensity[y * resolution + x];
                unsigned char brightness = (unsigned char)(intens * 255.0);
                
                int idx = (y * resolution + x) * 3;
                imgData[idx + 0] = brightness;
                imgData[idx + 1] = brightness;
                imgData[idx + 2] = brightness;
            }
        }
        
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/frame_%06d.png", outputDir.c_str(), frameNumber);
        
        if (!stbi_write_png(filename, resolution, resolution, 3, imgData.data(), resolution * 3)) {
            std::cerr << "[Frame " << frameNumber << "] FAILED to save" << std::endl;
        }
    }
};

class VideoConverter {
public:
    static bool CreateVideoFromFrames(const std::string& framesDir, 
                                      const std::string& outputFile,
                                      int fps = 30,
                                      const std::string& ffmpegPath = "ffmpeg") {
        std::cout << "\n========== CREATING VIDEO ==========" << std::endl;
        std::cout << "[  0%] Encoding video with libx264..." << std::endl;
        
        // Используем libx264 - самый универсальный кодек
        std::string cmd = "\"" + ffmpegPath + "\" -framerate " + std::to_string(fps) + 
                         " -i \"" + framesDir + "/frame_%06d.png\" " +
                         "-c:v libx264 -pix_fmt yuv420p -y \"" + outputFile + "\"";
        
        int result = system(cmd.c_str());
        
        if (result == 0) {
            std::cout << "[100%] Video created successfully!" << std::endl;
            std::cout << "✓ Output file: " << outputFile << std::endl;
            return true;
        } else {
            std::cerr << "ERROR: Video encoding failed" << std::endl;
            return false;
        }
    }
    
    static bool DeleteFramesDirectory(const std::string& framesDir) {
        std::cout << "\n========== CLEANUP ==========" << std::endl;
        std::cout << "Deleting temporary frames directory..." << std::endl;
        
        try {
            size_t removed = fs::remove_all(framesDir);
            std::cout << "✓ Deleted " << removed << " files" << std::endl;
            return true;
        } catch (const std::exception& ex) {
            std::cerr << "WARNING: Could not delete frames directory: " << ex.what() << std::endl;
            return false;
        }
    }
};

int main(int argc, char* argv[]) {
    std::cout << "=== String Art Video Generator v3.0 (Final) ===" << std::endl << std::endl;
    
    GenerationConfig config;
    
    if (argc < 2) {
        std::cout << "Usage: video_generator <result.json> [fps] [output_file]" << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  video_generator result.json" << std::endl;
        std::cout << "  video_generator result.json 60 output.mp4" << std::endl;
        return 1;
    }
    
    config.jsonPath = argv[1];
    if (argc > 2) config.fps = std::atoi(argv[2]);
    
    std::string outputFile = "output.mp4";
    if (argc > 3) outputFile = argv[3];
    
    if (!fs::exists(config.outputDir)) {
        fs::create_directories(config.outputDir);
        std::cout << "✓ Created output directory: " << config.outputDir << std::endl;
    }
    
    SimpleJsonParser loader;
    if (!loader.Load(config.jsonPath)) {
        return 1;
    }
    
    std::vector<Nail> nails = GenerateNails(
        loader.nailCount,
        config.imageResolution / 2.0,
        config.imageResolution / 2.0,
        config.imageResolution / 2.0 - 5
    );
    
    std::cout << "\n========== GENERATING FRAMES ==========" << std::endl;
    
    VideoFrameGenerator generator(config.imageResolution, config.lineAlpha, config.outputDir);
    generator.SetNails(nails);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    int frameCount = 0;
    int totalThreads = loader.threadSequence.size() / 2;
    
    for (int lineIdx = 0; lineIdx < totalThreads; lineIdx++) {
        int fromNail = loader.threadSequence[lineIdx * 2];
        int toNail = loader.threadSequence[lineIdx * 2 + 1];
        
        generator.ApplyLineWithAlpha(fromNail, toNail);
        
        if ((lineIdx + 1) % config.frameSkip == 0) {
            generator.SaveFrame(frameCount);
            frameCount++;
            
            if ((lineIdx + 1) % (config.frameSkip * 50) == 0) {
                int percent = (int)((lineIdx + 1) / (double)totalThreads * 100);
                std::cout << "[" << std::setw(3) << percent << "%] "
                          << (lineIdx + 1) << " / " << totalThreads << " lines" << std::endl;
            }
        }
    }
    
    generator.SaveFrame(frameCount);
    frameCount++;
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "\n========== FRAME GENERATION COMPLETE ==========" << std::endl;
    std::cout << "Total frames generated: " << frameCount << std::endl;
    std::cout << "Total processing time: " << duration.count() << "ms" << std::endl;
    
    bool videoCreated = VideoConverter::CreateVideoFromFrames(config.outputDir, outputFile, config.fps, config.ffmpegPath);
    
    if (videoCreated) {
        if (config.deleteFramesAfter) {
            VideoConverter::DeleteFramesDirectory(config.outputDir);
        }
        
        std::cout << "\n========== SUCCESS ==========" << std::endl;
        std::cout << "✓ Video ready: " << outputFile << std::endl;
        std::cout << "✓ FPS: " << config.fps << std::endl;
        std::cout << "\nAll done! Video file is ready to watch." << std::endl;
    } else {
        std::cerr << "\n========== ERROR ==========" << std::endl;
        std::cerr << "Failed to create video." << std::endl;
        std::cerr << "Frames are still in: " << config.outputDir << std::endl;
        return 1;
    }
    
    return 0;
}