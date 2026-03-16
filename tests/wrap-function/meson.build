libgreeter = shared_library(
  'greeter',
  'greeter.c',
  include_directories: '.',
)

test(
  'wrap-function',
  executable(
    'wrap-function',
    'main.c',
    include_directories: '.',
    dependencies: libglib_mock_dep,
    link_with: libgreeter,
  ),
)
