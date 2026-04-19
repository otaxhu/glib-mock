libfoo = shared_library(
  'foo',
  'foo.c',
  include_directories: '.',
)

libbar = shared_library(
  'bar',
  'bar.c',
  include_directories: '.',
)

test(
  'multiple-mocks',
  executable(
    'multiple-mocks',
    'main.c',
    include_directories: '.',
    dependencies: libglib_mock_dep,
    link_with: [libfoo, libbar],
  ),
)
