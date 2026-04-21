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
