project(
  'glib-mock',
  'c',
  version: '0.1.1-devel',
)

pkg = import('pkgconfig')

cc = meson.get_compiler('c')

c_args = [
  '-DG_LOG_DOMAIN="GLib-Mock"',
]

host_system = host_machine.system()

if host_system == 'windows' or host_system == 'cygwin'
  add_project_arguments(
    ['-DUNICODE', '-D_UNICODE'],
    language: 'c',
  )
endif

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

libdir = get_option('prefix') / get_option('libdir')
profile = get_option('profile')

subdir('src')

if get_option('tests')
  subdir('tests')
endif

if get_option('documentation')
  subdir('doc')
endif
