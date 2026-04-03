# Build glib-mock-priv

if host_system == 'darwin' or host_system == 'linux'
  libglib_mock_priv = shared_library(
    'glib-mock-priv',
    'glib-mock-priv.c',
    include_directories: '.',
    dependencies: libglib_dep,
    install: true,
  )
else
  # (optimization) Since darwin/linux-dl-preload aren't being built, there is no need
  # for a shared library.
  libglib_mock_priv = static_library(
    'glib-mock-priv',
    'glib-mock-priv.c',
    include_directories: '.',
    dependencies: libglib_dep,
  )
endif

dl_preload_path = ''

# Build darwin-dl-preload

if host_system == 'darwin'
  libglib_mock_darwin_dl_preload = shared_library(
    'glib-mock-darwin-dl-preload',
    'darwin-dl-preload.c',
    include_directories: '.',
    dependencies: [libglib_dep, libdl_dep],
    install: true,
    link_with: libglib_mock_priv,
  )

  if profile == 'installing'
    dl_preload_path = libdir / 'lib' + libglib_mock_darwin_dl_preload.name() + '.dylib'
  elif profile == 'development'
    dl_preload_path = libglib_mock_darwin_dl_preload.full_path()
  endif
endif

# Build linux-dl-preload

if host_system == 'linux'
  libglib_mock_linux_dl_preload = shared_library(
    'glib-mock-linux-dl-preload',
    'linux-dl-preload.c',
    include_directories: '.',
    dependencies: [libglib_dep, libdl_dep],
    install: true,
    link_with: libglib_mock_priv,
  )

  if profile == 'installing'
    dl_preload_path = libdir / 'lib' + libglib_mock_linux_dl_preload.name() + '.so'
  elif profile == 'development'
    dl_preload_path = libglib_mock_linux_dl_preload.full_path()
  endif
endif

# Build config.h

cfg_config_h = configuration_data()

cfg_config_h.set_quoted('dl-preload-path', dl_preload_path)

configure_file(
  input: 'config.h.in',
  output: 'config.h',
  configuration: cfg_config_h,
)

# Build glib-mock, generate pkg-config and install headers

libglib_mock = library(
  'glib-mock',
  'glib-mock.c',
  include_directories: '.',
  c_args: c_args,
  dependencies: [libglib_dep, libdl_dep],
  install: true,
  link_with: libglib_mock_priv,
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
