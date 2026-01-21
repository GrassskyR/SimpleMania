#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // 命令行谱面生成器
    if (argc < 6) {
        std::cout << "Usage: chartgen <bpm> <duration_ms> <key_count> <density> <output.osu>\n";
        std::cout << "Example: chartgen 120 30000 4 2 demo.osu\n";
        return 1;
    }

    double bpm = std::atof(argv[1]);
    int durationMs = std::atoi(argv[2]);
    int keyCount = std::atoi(argv[3]);
    double density = std::atof(argv[4]);
    std::string outputPath = argv[5];

    if (bpm <= 0.0 || durationMs <= 0 || keyCount <= 0 || density <= 0.0) {
        std::cout << "Invalid arguments.\n";
        return 1;
    }

    double beatLength = 60000.0 / bpm;
    double interval = beatLength / density;
    int noteCount = static_cast<int>(durationMs / interval);

    std::ofstream out(outputPath);
    if (!out.is_open()) {
        std::cout << "Failed to write output file.\n";
        return 1;
    }

    out << "osu file format v14\n\n";
    out << "[General]\n";
    out << "AudioFilename: demo.wav\n";
    out << "Mode: 3\n\n";
    out << "[Metadata]\n";
    out << "Title: SimpleMania Demo\n";
    out << "Artist: CLI Generator\n\n";
    out << "[Difficulty]\n";
    out << "CircleSize: " << keyCount << "\n";
    out << "OverallDifficulty: 5\n\n";
    out << "[TimingPoints]\n";
    out << "0," << beatLength << ",4,2,0,100,1,0\n\n";
    out << "[HitObjects]\n";

    int lane = 0;
    for (int i = 0; i < noteCount; ++i) {
        int timeMs = static_cast<int>(i * interval);
        int x = static_cast<int>((lane + 0.5) * 512.0 / keyCount);
        int y = 192;
        int type = 1;
        int hitSound = 0;
        out << x << ',' << y << ',' << timeMs << ',' << type << ',' << hitSound << ",0:0:0:0:\n";
        lane = (lane + 1) % keyCount;
    }

    std::cout << "Generated: " << outputPath << "\n";
    return 0;
}
