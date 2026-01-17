#pragma once
#include <vector>
#include <algorithm>

class Image {
private:
    int width = 0;
    int height = 0;
    std::vector<double> data;

public:
    Image() = default;
    Image(int w, int h) : width(w), height(h), data(w * h, 0.0) {}
    inline double& at(int x, int y) {
        return data[y * width + x];
    }
    inline double at(int x, int y) const {
        return data[y * width + x];
    }
    inline double& atSafe(int x, int y) {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            static double dummy = 0.0;
            return dummy;
        }
        return data[y * width + x];
    }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getSize() const { return width * height; }
    std::vector<double>& getData() { return data; }
    const std::vector<double>& getData() const { return data; }
    Image(const Image& other) = default;
    Image& operator=(const Image& other) = default;
    Image(Image&& other) noexcept = default;
    Image& operator=(Image&& other) noexcept = default;
    ~Image() = default;

    void fill(double value) {
        std::fill(data.begin(), data.end(), value);
    }

    void resize(int w, int h) {
        width = w;
        height = h;
        data.assign(w * h, 0.0);
    }
};