execv_bug_env = environment()
execv_bug_env.prepend('PATH', meson.current_build_dir())

execv_bug = executable(
  'execv-bug',
  'execv-bug.c',
  dependencies: [libglib_mock_dep],
)

test(
  execv_bug.name(),
  find_program('env'),
  args: [execv_bug.name()],
  env: execv_bug_env,
)

libmy_fwrite = shared_library(
  'my-fwrite',
  'my-fwrite.c',
)

subdir('wrap-function')

if host_system == 'linux' or host_system == 'darwin'
  subdir('unix')
endif

if host_system == 'windows' or host_system == 'cygwin'
  subdir('win32')
endif
