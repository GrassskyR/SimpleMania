# SimpleMania

这是一个**纯 vibe coding 项目**：用最少的工程成本搭建一个可运行的下落式音游 Demo。

功能概览：
- 读取 osu!mania `.osu` 谱面（从 `assets/<folder>/*.osu`）
- 判定（Perfect/Good/Miss）与分数/连击/ACC 统计
- 下落速度可调
- 菜单选择谱面、暂停与倒计时
- SDL2 + CMake

## 运行说明

把谱面放到：
```
assets/<任意文件夹>/你的谱面.osu
```
音频文件需与 `.osu` 在同一目录。

## Windows (MSYS2 MinGW64)

### 依赖安装
在 MSYS2 MinGW64 终端：
```
pacman -S --needed \
  mingw-w64-x86_64-toolchain \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-SDL2 \
  mingw-w64-x86_64-SDL2_mixer
```

### 构建
```
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/msys64/mingw64
cmake --build build
```

### 运行
```
build/simplemania.exe
```

## Ubuntu (22.04+)

### 依赖安装
```
sudo apt update
sudo apt install -y build-essential cmake libsdl2-dev libsdl2-mixer-dev
```

### 构建
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 运行
```
./build/simplemania
```

## 操作说明

- 菜单：`Up/Down` 选择，`Enter` 开始
- 游戏中：`ESC` 暂停，`Up/Down` 选择暂停菜单
- 速度：`Ctrl +` / `Ctrl -`

默认键位：
- 4K: `D F J K`
- 5K: `D F Space J K`
- 6K: `S D F J K L`
- 7K: `S D F Space J K L`

## 备注

这是一个最小可运行的 Demo 架构，适合在此基础上继续扩展判定逻辑、音效、皮肤、编辑器等功能。
