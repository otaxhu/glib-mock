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

if host_system == 'windows' or host_system == 'cygwin'
  multiple_mocks_link_args = [] # empty
else
  multiple_mocks_link_args = [
    '-Wl,--no-as-needed',
    '-L' + meson.current_build_dir(),
    '-lfoo',
    '-lbar',
    '-Wl,--as-needed',
  ]
endif

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
    link_args: multiple_mocks_link_args,
  ),
)
