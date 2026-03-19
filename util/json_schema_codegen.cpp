#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "json_schema_codegen.hpp"

namespace {

struct cli_options {
  std::filesystem::path      schema_path;
  std::optional<std::string> root_name;
  std::optional<std::filesystem::path> output_path;
};

auto print_usage(std::string_view program_name) -> void {
  std::cerr << "Usage: " << program_name << " <schema.json> [--root NAME] [--output FILE]\n";
}

auto parse_arguments(int argc, char** argv) -> std::expected<cli_options, std::string> {
  if (argc < 2) {
    return std::unexpected("schema file path is required");
  }

  cli_options options{};
  bool schema_was_set = false;

  for (int index = 1; index < argc; ++index) {
    auto const arg = std::string_view{argv[index]};
    if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      std::exit(0);
    }

    if (arg == "--root") {
      if (index + 1 >= argc) {
        return std::unexpected("`--root` requires a value");
      }
      options.root_name = argv[++index];
      continue;
    }

    if (arg == "--output") {
      if (index + 1 >= argc) {
        return std::unexpected("`--output` requires a value");
      }
      options.output_path = std::filesystem::path{argv[++index]};
      continue;
    }

    if (schema_was_set) {
      return std::unexpected("unexpected positional argument: " + std::string{arg});
    }

    options.schema_path = std::filesystem::path{arg};
    schema_was_set = true;
  }

  if (!schema_was_set) {
    return std::unexpected("schema file path is required");
  }

  return options;
}

}  // namespace

auto main(int argc, char** argv) -> int {
  auto parsed = parse_arguments(argc, argv);
  if (!parsed) {
    print_usage(argc > 0 ? argv[0] : "glz_util_json_schema_codegen");
    std::cerr << parsed.error() << '\n';
    return 1;
  }

  glz_util::schema_codegen::codegen_options options{};
  if (parsed->root_name.has_value()) {
    options.root_name = *parsed->root_name;
  }

  auto generated = glz_util::schema_codegen::generate_structs_from_json_schema_file(parsed->schema_path, options);
  if (!generated) {
    std::cerr << generated.error() << '\n';
    return 1;
  }

  if (parsed->output_path.has_value()) {
    std::ofstream output{*parsed->output_path};
    if (!output) {
      std::cerr << "failed to open output file `" << parsed->output_path->string() << "`\n";
      return 1;
    }
    output << *generated;
    return output.good() ? 0 : 1;
  }

  std::cout << *generated;
  return 0;
}
