// #include "datasource.h"
// #include <fstream>

// void PpmStorage::save(const ICanvas& canvas, const std::string& path) {
//     std::ofstream file(path);
//     if (!file.is_open()) return;
    
//     // Простой заголовок: ширина высота
//     file << canvas.getWidth() << " " << canvas.getHeight() << "\n";
    
//     // Пишем 1 или 0 для каждого пикселя
//     for (int y = 0; y < canvas.getHeight(); ++y) {
//         for (int x = 0; x < canvas.getWidth(); ++x) {
//             Pixel p = canvas.getPixel(x, y);
//             file << (p.filled ? 1 : 0) << " ";
//         }
//         file << "\n";
//     }
// }

// void PpmStorage::load(ICanvas& canvas, const std::string& path) {
//     // Заглушка
// }