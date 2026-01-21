#pragma once

#include <string>

#include "Chart.h"

// 读取osu!mania谱面并填充Chart结构
bool ParseOsuFile(const std::string& path, Chart& outChart, std::string& error);
