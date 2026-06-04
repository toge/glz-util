# AGENTS.md — glz-util

## 概要

Glaze のメタ情報を利用して環境変数やコマンドライン引数を C++ 構造体へ読み込む header-only ライブラリ。`include/glz-util/` にヘッダ、`util/` にコード生成CLI、`test/` に Catch2 テストを配置。

## ビルドコマンド

```bash
./build.sh          # vcpkg 使用、Release ビルド
./build.sh static   # static ビルド (build_static/)
```

- `VCPKG_ROOT` は `~/vm/vcpkg` にハードコードされている（`build.sh:30`）
- CMake 3.25 以上、C++23 以上（C++26 優先）
- vcpkg の triplet は `x64-linux-static`（`build.sh:33`）

## テスト

```bash
./build.sh
cd build && ctest --output-on-failure
```

- Catch2 を使用、テスト実行は `build/test/all_test`
- テスト対象: `env`, `args`, `diff`, `print`, `st_tree`, `tsl-hat-trie`, `zfp`, `json_schema_codegen`
- テスト内で `setenv()` を使うため、環境変数の状態に依存

## コードスタイル

- clang-format: LLVM ベース、インデント 2 スペース、列幅 200
- ポインタ左寄せ (`int* p`)
- 宣言・代入の連続アライメント有効
- .editorconfig: UTF-8、LF、末尾改行あり、トリムあり

## ライブラリ構造

- `include/glz-util/env.hpp` — `from_env<T>()` 環境変数読み込み
- `include/glz-util/args.hpp` — `from_args<T>(argc, argv)` コマンドライン引数解析
- `include/glz-util/diff.hpp` — `diff_members(before, after)` 構造体差分比較
- `include/glz-util/print.hpp` — `print_members(value)` メンバー一覧表示
- `include/glz-util/st_tree.hpp` — st_tree と Glaze の相互運用 wrapper
- `include/glz-util/tsl-hat-trie.hpp` — HAT-trie と Glaze の相互運用 wrapper
- `include/glz-util/zfp.hpp` — ZFP と Glaze の相互運用 wrapper
- `util/json_schema_codegen.cpp` — JSON Schema から struct 定義を生成する CLI ツール

## 依存関係

- `glaze` — 公開依存（CMake: `glaze::glaze`）
- `catch2` — テスト依存
- `st-tree`, `tsl-hat-trie`, `zfp` — wrapper 利用時に個別解決（CMakeLists.txt の公開依存には含まれない）

## 注意事項

- `from_env` / `from_args` の戻り値は `std::expected` 型。エラー時は `std::unexpected` を返す
- `from_args` のエラーは `ArgsError` 型で、`is_help_requested()` で help モード要求を区別できる
- エラー文字列は JSON 形式で機械処理可能（例: `{"env":"KEY","value":"abc","detail":"..."}`）
- `zfp.hpp` を使う場合、利用側で `find_package(zfp CONFIG REQUIRED)` が必要
- `st_tree.hpp` / `tsl-hat-trie.hpp` を使う場合、利用側でヘッダ検索パスの設定が必要
- `json_schema_codegen` は `$ref` のローカル参照のみ対応。`oneOf` / `anyOf` / `allOf` / 再帰参照は未対応
