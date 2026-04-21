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

test(
  'multiple-get-real-rt-dynamic-linked',
  executable(
    'multiple-get-real-rt-dynamic-linked',
    'multiple-get-real-rt-dynamic-linked.c',
    include_directories: '.',
    dependencies: libglib_mock_dep,
  ),
  env: {
    'LIB_FOO_PATH': libfoo.full_path(),
    'LIB_BAR_PATH': libbar.full_path(),
  },
)

subdir('rt-dynamic-linked')
subdir('deep-rt-dynamic-linked')
