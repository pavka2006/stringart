#pragma once
#include "models.h"
#include "array2d.h"
#include <fstream>
#include <vector>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

class ImageProcessor {
public:
    Image LoadAndProcess(const std::string& path, int size) {
        int width, height, channels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 3);

        Array2D<unsigned char> resized(size * 3, size);

        float scaleX = (float)width / size;
        float scaleY = (float)height / size;

        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                int srcX = (int)(x * scaleX);
                int srcY = (int)(y * scaleY);
                int srcIdx = (srcY * width + srcX) * 3;

                resized(y, x * 3 + 0) = data[srcIdx + 0];
                resized(y, x * 3 + 1) = data[srcIdx + 1];
                resized(y, x * 3 + 2) = data[srcIdx + 2];
            }
        }

        stbi_image_free(data);

        Image matrix(size, size);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                unsigned char r = resized(y, x * 3 + 0);
                unsigned char g = resized(y, x * 3 + 1);
                unsigned char b = resized(y, x * 3 + 2);

                double gray = 0.299 * r + 0.587 * g + 0.114 * b;
                matrix.at(x, y) = 255.0 - gray;
            }
        }

        return matrix;
    }
};

class Exporter {
public:
    void ExportJson(const GenerationResult& result, const std::string& path) {
        std::ofstream file(path);
        file << "{\n";
        file << " \"nail_count\": " << result.nails.size() << ",\n";
        file << " \"total_lines\": " << result.lineSequence.size() << ",\n";
        file << " \"thread_sequence\": [\n";

        for (size_t i = 0; i < result.lineSequence.size(); i++) {
            const auto& line = result.lineSequence[i];
            if (i == 0) {
                file << " " << line.fromNailId << ",\n";
            }

            file << " " << line.toNailId;
            if (i < result.lineSequence.size() - 1) {
                file << ",";
            }

            file << "\n";
        }

        file << " ]\n";
        file << "}\n";
        file.close();
    }

    void ExportPng(const GenerationResult& result, const std::string& path, int size) {
        std::vector<unsigned char> imgData(size * size * 3);

        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                double intensity = result.renderedImage.at(size - 1 - x, y);
                unsigned char brightness = (unsigned char)(intensity * 255.0);

                int idx = (y * size + x) * 3;
                imgData[idx + 0] = brightness;
                imgData[idx + 1] = brightness;
                imgData[idx + 2] = brightness;
            }
        }

        stbi_write_png(path.c_str(), size, size, 3, imgData.data(), size * 3);
    }
};