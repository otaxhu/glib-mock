libgreeter = shared_library(
  'greeter',
  'greeter.c',
  include_directories: '.',
)

testenv = environment(libglib_mock_dep.get_variable('testenv'))

test(
  'wrap-function',
  executable(
    'wrap-function',
    'main.c',
    include_directories: '.',
    dependencies: libglib_mock_dep,
    link_with: libgreeter,
  ),
  env: testenv,
)
