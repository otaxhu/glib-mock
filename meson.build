project('glib-mock', 'c')

glib_mock_incs = [
  'src/glib-mock.h',
]

libglib_mock_dep = declare_dependency(
  include_directories: include_directories('src'),
  dependencies: dependency('glib-2.0'),
)

meson.override_dependency('glib-mock', libglib_mock_dep)

install_headers('src/glib-mock.h', subdir: 'glib-mock')

glib_mock_run = configure_file(
  input: 'glib-mock-run.py',
  output: 'glib-mock-run',
  install: not meson.is_subproject(),
  install_dir: get_option('bindir'),
  copy: true,
)

meson.override_find_program('glib-mock-run', glib_mock_run)
