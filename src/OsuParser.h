#pragma once

#include <string>

#include "Chart.h"

bool ParseOsuFile(const std::string& path, Chart& outChart, std::string& error);
