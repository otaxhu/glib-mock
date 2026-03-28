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
