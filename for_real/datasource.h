// #pragma once
// #include "domain.h"
// #include <string>

// // Интерфейс слоя данных
// class IImageStorage {
// public:
//     virtual ~IImageStorage() = default;
//     virtual void save(const ICanvas& canvas, const std::string& path) = 0;
//     virtual void load(ICanvas& canvas, const std::string& path) = 0;
// };

// // Реализация для сохранения в PPM
// class PpmStorage : public IImageStorage {
// public:
//     void save(const ICanvas& canvas, const std::string& path) override;
//     void load(ICanvas& canvas, const std::string& path) override;
// };