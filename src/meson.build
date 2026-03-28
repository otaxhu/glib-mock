# Build glib-mock, generate pkg-config and install headers

libglib_mock = library(
  'glib-mock',
  'glib-mock.c',
  include_directories: '.',
  c_args: c_args,
  dependencies: [libglib_dep, libdl_dep],
  install: true,
)

libglib_mock_dep = declare_dependency(
  include_directories: '.',
  dependencies: [libglib_dep, libdl_dep],
  link_with: libglib_mock,
)

pkg.generate(
  libglib_mock,
  requires: ['glib-2.0'],
  name: 'GLib Mock',
  description: 'Mocking framework for unit testing',
  filebase: 'glib-mock',
  subdirs: 'glib-mock',
)

meson.override_dependency('glib-mock', libglib_mock_dep)

install_headers('glib-mock.h', subdir: 'glib-mock')
