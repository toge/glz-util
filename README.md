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
- `glz_util_json_schema_codegen`
    - JSON Schema から `struct` と `glz::meta` 定義を自動生成できます。
- [HAT-trie](https://github.com/Tessil/hat-trie)とglazeとの相互運用のためのwrapperを提供しています。
- [ZFP](https://github.com/LLNL/ZFP)とglazeとの相互運用のためのwrapperを提供しています。

## 必要環境

- CMake（3.25以上）
- C++23以上（利用可能ならC++26を優先）
- 依存ライブラリ
    - `glaze`
    - `zfp`
    - `catch2`（テスト時）

このリポジトリには `vcpkg.json` が含まれており、依存関係として `glaze` / `zfp` / `catch2` を宣言しています。

## 使い方

### 環境変数から構造体を生成

```cpp
#include <string>
#include "glz-util/env.hpp"

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

  if (!cfg) {
    // 例: エラーメッセージを出力して終了
    // std::cerr << cfg.error() << std::endl;
    return 1;
  }

  auto const& value = *cfg;
}
```

### オプション付き読み込み（例: JSON）

```cpp
auto cfg = glz_util::from_env<glz::opts{.format = glz::JSON}, AppConfig>();

if (!cfg) {
  // エラー時はstd::unexpected<std::string>
  // cfg.error() で詳細を取得できます
}
```

> `from_env` の戻り値は `std::expected<T, std::string>` です。
> パース失敗時は最初のエラーで処理を停止し、`std::unexpected` を返します。

### エラー文字列の形式

失敗時の`error()`は、機械処理しやすいJSON文字列です。

```json
{"env":"NUMBER_1","value":"abc","detail":"Invalid argument"}
```

- `env`: 失敗した環境変数名
- `value`: 入力文字列
- `detail`: 失敗理由（`std::from_chars` / `glz::read` 由来）

### エラー文字列のパース例

```cpp
#include <iostream>
#include <string>

#include "glz-util/env.hpp"

struct EnvParseError {
  std::string env;
  std::string value;
  std::string detail;
};

template <>
struct glz::meta<EnvParseError> {
  using T = EnvParseError;
  static constexpr auto value = glz::object(
    "env", &T::env,
    "value", &T::value,
    "detail", &T::detail
  );
};

int main() {
  auto cfg = glz_util::from_env<AppConfig>();
  if (!cfg) {
    EnvParseError parsed{};
    if (auto ec = glz::read_json(parsed, cfg.error()); ec) {
      std::cerr << "failed to parse error payload: " << cfg.error() << '\n';
      return 1;
    }

    std::cerr << "env=" << parsed.env
              << " value=" << parsed.value
              << " detail=" << parsed.detail << '\n';
    return 1;
  }
}
```

### HAT-trieとglazeの相互運用

```cpp
#include <iostream>

#include "tsl/htrie_map.h"
#include "glz-util/tsl-hat-trie.hpp"

int main() {
  auto buffer = R"({"123":"hello","987":"world"})";
  auto target = htrie_map_wrapper<char, std::string>{};

  glz::read_json(target, buffer);

  auto const& target2 = target.raw();

  std::cout << target2.at("123") << '\n'; // => "hello"
  std::cout << target2.at("987") << '\n'; // => "world"
}
```

### ZFPとglazeの相互運用

```cpp
#include <iostream>
#include <string>
#include <vector>

#include "glz-util/zfp.hpp"

int main() {
  auto buffer = R"([[1.0,2.0],[3.0,4.0]])";
  auto target = zfp_array2_wrapper<double>{16.0};

  if (auto ec = glz::read_json(target, buffer); ec) {
    return 1;
  }

  auto const& raw = target.raw();
  std::cout << raw.size_x() << "x" << raw.size_y() << '\n';

  auto json = std::string{};
  if (auto ec = glz::write_json(target, json); ec) {
    return 1;
  }

  std::cout << json << '\n';
}
```

`zfp_array1_wrapper` 〜 `zfp_array4_wrapper` は、Glaze との境界では通常のJSON配列として振る舞います。
例えば `zfp_array3_wrapper<double>` は `[[[1.0,2.0],[3.0,4.0]]]` のようなネスト配列を読み書きできます。

### JSON Schema から struct を生成

`json_schema_codegen` は、JSON Schema ファイルを読み込んで、Glaze でそのまま JSON を読める C++ ヘッダ断片を生成します。

```bash
json_schema_codegen schema.json --root AppConfig --output app_config.hpp
```

標準出力へ出したい場合は `--output` を省略できます。

生成されるコードはおおむね次の形です。

```cpp
struct AppConfig {
  std::string name{};
  std::optional<std::int64_t> retry_count{};
};

template <>
struct glz::meta<AppConfig> {
  using T = AppConfig;
  static constexpr auto value = glz::object(
    "name", &T::name,
    "retry_count", &T::retry_count
  );
};
```

その後は通常どおり `glz::read_json` / `glz::read_file_json` で読み込めます。

```cpp
#include "app_config.hpp"

int main() {
  AppConfig config{};
  std::string buffer;
  if (auto ec = glz::read_file_json(config, "config.json", buffer); ec) {
    return 1;
  }
}
```

初期実装で対応している主な JSON Schema 要素:

- `type`
- `properties`
- `required`
- `items`
- `$ref`（ローカル参照のみ）
- `$defs` / `definitions` 経由の参照先オブジェクト
- `enum`（型推論用途）

未対応または明示的にエラーにする要素:

- `oneOf`
- `anyOf`
- `allOf`
- スキーマ値を持つ `additionalProperties`
- 再帰参照

## CMake での利用

このプロジェクトは `glz-util::glz-util` ターゲットを提供します。
ZFP wrapper を利用する場合は、依存先の `zfp` が `OpenMP::OpenMP_C` を公開するため、利用側プロジェクトも `project(... LANGUAGES C CXX)` のように C 言語を有効化してください。

```cmake
find_package(glz-util CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE glz-util::glz-util)
```

## テスト

テストはCatch2を利用しています。
`test/test_env.cpp` に、環境変数からの読み取り動作（通常モード・JSONオプション付き）が含まれています。

## ライセンス

MIT Licenseです。
詳細は `LICENSE` を参照してください。
