find_package(Catch2 REQUIRED CONFIG)
enable_language(C)
find_package(zfp CONFIG REQUIRED)
find_path(ST_TREE_INCLUDE_DIRS "st_tree.h")
find_path(TSL_HAT_TRIE_INCLUDE_DIRS "tsl/array-hash/array_growth_policy.h")

file(GLOB test_src test_*.cpp)

add_executable(all_test main.cpp ${test_src})

target_link_libraries(all_test PRIVATE
  Catch2::Catch2 Catch2::Catch2WithMain
  glz-util::glz-util
  zfp::zfp
)
target_include_directories(all_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/util
  ${ST_TREE_INCLUDE_DIRS}
  ${TSL_HAT_TRIE_INCLUDE_DIRS}
)
target_compile_features(all_test PRIVATE ${STD_CPP})

add_test(NAME all_test COMMAND all_test -r junit)
