libgreeter = shared_library(
  'greeter',
  'greeter.c',
  include_directories: '.',
  dependencies: libglib_dep,
)

test(
  'deep-rt-dynamic-linked',
  executable(
    'deep-rt-dynamic-linked',
    'main.c',
    include_directories: '.',
    dependencies: libglib_mock_dep,
  ),
  env: {
    'LIB_MY_FWRITE_PATH': libmy_fwrite.full_path(),
    'LIB_GREETER_PATH': libgreeter.full_path(),
  },
)
