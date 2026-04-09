test(
  'unmocked',
  executable(
    'unmocked',
    'unmocked.c',
    include_directories: '.',
    dependencies: libglib_mock_dep,
  ),
  env: {
    'LIB_MY_FWRITE_PATH': libmy_fwrite.full_path(),
  },
)

subdir('rt-dynamic-linked')
subdir('deep-rt-dynamic-linked')
