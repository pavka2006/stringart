#pragma once

#include <vector>
#include <cstddef>

template <typename T>
class Array2D {
private:
    std::vector<T> data;
    size_t width;
    size_t height;

public:
    Array2D(size_t w, size_t h) : width(w), height(h), data(w * h) {}

    T& operator()(size_t row, size_t col) {
        return data[row * width + col];
    }

    const T& operator()(size_t row, size_t col) const {
        return data[row * width + col];
    }

    size_t getWidth() const { return width; }
    size_t getHeight() const { return height; }
    size_t getSize() const { return width * height; }

    T* data_ptr() { return data.data(); }
    const T* data_ptr() const { return data.data(); }

    void fill(const T& value) {
        std::fill(data.begin(), data.end(), value);
    }

    void clear() {
        data.clear();
    }

    std::vector<T>& getRawData() { return data; }
    const std::vector<T>& getRawData() const { return data; }
};