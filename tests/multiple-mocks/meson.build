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
    # FIXME: Fix this link_args bs :/
    #
    # This arguments forces the linkage against libfoo.so and libbar.so objects,
    # which is needed in order to g_mock_get_real to work properly for this test
    link_args: [
      '-Wl,-L' + meson.current_build_dir(),
      '-Wl,--no-as-needed',
      '-Wl,-lfoo',
      '-Wl,-lbar',
      '-Wl,--as-needed',
    ],
  ),
)
