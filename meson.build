project('glib-mock', 'c')

pkg = import('pkgconfig')

cc = meson.get_compiler('c')

c_args = [
  '-DG_LOG_DOMAIN="GLib-Mock"',
]

host_system = host_machine.system()

dlopen_dlsym_test_code = '''
#include <dlfcn.h>
#include <stdio.h>
int r;
int glib_underscore_test (void) { return 42; }
int main (int argc, char ** argv) {
  void *f1 = (void*)0, *f2 = (void*)0, *handle;
  handle = dlopen ((void*)0, 0);
  if (handle) {
    f1 = dlsym (handle, "glib_underscore_test");
    f2 = dlsym (handle, "_glib_underscore_test");
  }
  r = (!f2 || f1) ? puts ("1") : puts ("0");
  return r > 0 ? 0 : r;
}'''

libdl_dep = []
libglib_dep = dependency('glib-2.0')

if host_system == 'windows' or cc.links(
  dlopen_dlsym_test_code,
  name: 'dlopen() and dlsym() in system libraries',
)
  # Do nothing
elif cc.links(
  dlopen_dlsym_test_code,
  args: '-ldl',
  name: 'dlopen() and dlsym() in libdl',
)
  libdl_dep = cc.find_library('dl')
else
  error(
    'This platform is missing libdl functions which are required for this library',
  )
endif

srcs = [
  'src/glib-mock.c',
]

if host_system == 'windows'
  srcs += 'src/glib-mock-win32.c'
endif

libglib_mock = library(
  'glib-mock',
  srcs,
  include_directories: 'src',
  c_args: c_args,
  dependencies: [libglib_dep, libdl_dep],
  install: true,
)

libglib_mock_vars = [
  'testenv=DYLD_FORCE_FLAT_NAMESPACE=1',
]

libglib_mock_dep = declare_dependency(
  include_directories: 'src',
  dependencies: [libglib_dep, libdl_dep],
  link_with: libglib_mock,
  variables: libglib_mock_vars,
)

pkg.generate(
  libglib_mock,
  requires: ['glib-2.0'],
  name: 'GLib Mock',
  description: 'Mocking framework for unit testing',
  filebase: 'glib-mock',
  subdirs: 'glib-mock',
  variables: libglib_mock_vars,
)

meson.override_dependency('glib-mock', libglib_mock_dep)

install_headers('src/glib-mock.h', subdir: 'glib-mock')

subdir('tests')
