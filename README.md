# glz-util

`glz-util` は、[Glaze](https://github.com/stephenberry/glaze) のメタ情報を利用して、**環境変数を C++ の構造体へ読み込む**ための軽量ユーティリティです。
このリポジトリではheader onlyライブラリとして提供しています。

## 特徴

- `glz_util::from_env<T>()`
    - `glz::meta<T>`で定義したキー名を環境変数名として解釈し、構造体`T`に値を読み込みます。
- `glz_util::from_env<Opts, T>()`
    - `glz::opts`を指定した読み込み（例: JSON形式）に対応します。
- `glz_util::print_members(value)`
    - 構造体メンバー名と値を列挙して表示できます。

## 必要環境

- CMake（3.25以上）
- C++23以上（利用可能ならC++26を優先）
- 依存ライブラリ
    - `glaze`
    - `catch2`（テスト時）

このリポジトリには `vcpkg.json` が含まれており、依存関係として `glaze` / `catch2` を宣言しています。

## 使い方

### 環境変数から構造体を生成

```cpp
#include <string>
#include "glz_util/env.hpp"

struct AppConfig {
  int         number1 = 0;
  float       number2 = 0.0f;
  std::string message;
};

template <>
struct glz::meta<AppConfig> {
  using T = AppConfig;
  static constexpr auto value = glz::object(
    "NUMBER_1", &T::number1,
    "NUMBER_2", &T::number2,
    "MESSAGE",  &T::message
  );
};

int main() {
  // 例: export NUMBER_1=42
  auto cfg = glz_util::from_env<AppConfig>();
}
```

### オプション付き読み込み（例: JSON）

```cpp
auto cfg = glz_util::from_env<glz::opts{.format = glz::JSON}, AppConfig>();
```

## CMake での利用

このプロジェクトは `glz_util::glz_util` ターゲットを提供します。

```cmake
find_package(glz_util CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE glz_util::glz_util)
```

## テスト

テストはCatch2を利用しています。
`test/test_env.cpp` に、環境変数からの読み取り動作（通常モード・JSONオプション付き）が含まれています。

## ライセンス

MIT License です。
詳細は `LICENSE` を参照してください。
