#include "OsuParser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <vector>

namespace {
std::string Trim(const std::string& text) {
    size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }
    size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(start, end - start);
}

std::vector<std::string> Split(const std::string& text, char delim) {
    std::vector<std::string> parts;
    std::string item;
    std::stringstream stream(text);
    while (std::getline(stream, item, delim)) {
        parts.push_back(item);
    }
    return parts;
}

int ParseInt(const std::string& value, int fallback = 0) {
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

double ParseDouble(const std::string& value, double fallback = 0.0) {
    try {
        return std::stod(value);
    } catch (...) {
        return fallback;
    }
}
}

bool ParseOsuFile(const std::string& path, Chart& outChart, std::string& error) {
    std::ifstream file(path);
    if (!file.is_open()) {
        error = "Failed to open osu file: " + path;
        return false;
    }

    std::string section;
    int mode = -1;
    bool hasTimingBpm = false;
    outChart = Chart();

    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line.rfind("//", 0) == 0) {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            continue;
        }

        if (section == "General" || section == "Difficulty" || section == "Metadata") {
            auto split = Split(line, ':');
            if (split.size() < 2) {
                continue;
            }
            std::string key = Trim(split[0]);
            std::string value = Trim(line.substr(line.find(':') + 1));

            if (section == "General") {
                if (key == "AudioFilename") {
                    outChart.audioFilename = value;
                } else if (key == "Mode") {
                    mode = ParseInt(value, -1);
                }
            } else if (section == "Difficulty") {
                if (key == "CircleSize") {
                    outChart.keyCount = std::max(1, ParseInt(value, outChart.keyCount));
                }
            } else if (section == "Metadata") {
                if (key == "Title") {
                    outChart.title = value;
                } else if (key == "Artist") {
                    outChart.artist = value;
                }
            }
            continue;
        }

        if (section == "TimingPoints") {
            auto split = Split(line, ',');
            if (split.size() >= 2) {
                TimingPoint point;
                point.timeMs = ParseDouble(split[0]);
                point.beatLengthMs = ParseDouble(split[1]);
                point.meter = split.size() >= 3 ? ParseInt(split[2], 4) : 4;
                point.inherited = point.beatLengthMs < 0.0;
                outChart.timingPoints.push_back(point);
                if (!point.inherited && point.beatLengthMs > 0.0 && !hasTimingBpm) {
                    outChart.baseBpm = 60000.0 / point.beatLengthMs;
                    hasTimingBpm = true;
                }
            }
            continue;
        }

        if (section == "HitObjects") {
            auto split = Split(line, ',');
            if (split.size() < 5) {
                continue;
            }
            int x = ParseInt(split[0]);
            int timeMs = ParseInt(split[2]);
            int type = ParseInt(split[3]);
            bool isHold = (type & 128) != 0;
            int lane = 0;
            if (outChart.keyCount > 0) {
                lane = static_cast<int>(x * outChart.keyCount / 512);
                if (lane < 0) {
                    lane = 0;
                } else if (lane >= outChart.keyCount) {
                    lane = outChart.keyCount - 1;
                }
            }
            int endTimeMs = timeMs;
            if (isHold && split.size() >= 6) {
                std::string params = split[5];
                auto colon = params.find(':');
                if (colon != std::string::npos) {
                    endTimeMs = ParseInt(params.substr(0, colon), timeMs);
                }
            }

            Note note;
            note.lane = lane;
            note.timeMs = timeMs;
            note.endTimeMs = endTimeMs;
            note.isHold = isHold;
            outChart.notes.push_back(note);
        }
    }

    if (mode != -1 && mode != 3) {
        error = "Only osu!mania (Mode=3) is supported.";
        return false;
    }

    std::sort(outChart.notes.begin(), outChart.notes.end(), [](const Note& a, const Note& b) {
        return a.timeMs < b.timeMs;
    });

    if (outChart.notes.empty()) {
        error = "No notes found in osu file.";
        return false;
    }
    return true;
}
