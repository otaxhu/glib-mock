libgreeter = library(
  'greeter',
  'greeter.c',
  include_directories: '.',
  dependencies: libglib_dep,
)

test(
  'rt-dynamic-linked',
  executable(
    'rt-dynamic-linked',
    'main.c',
    include_directories: '.',
    dependencies: libglib_mock_dep,
    link_with: libgreeter,
  ),
)
