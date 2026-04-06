# GLib Mock (Mocking framework for unit testing)

GLib Mock is a framework for mocking non-GObject C functions, aiming to be added to the GLib Testing framework as the standardized way to create mocks.

## Features

- ### Function interception (Shim Pattern)

  Intercept standard functions or system calls and redirect them to mock implementations. This allows you to monitor data flow, alter behavior, or simulate various kernel responses without modifying the original source code. Mocks can either replace the target functionality entirely or act as a "shim" that wraps and calls the real implementation. For an example on how this works, please refer to [wrap-function](./tests/wrap-function) test.

- ### Predictive unit testing

  If you are familiar with unit testing in any programming language, you will know that some projects may have dependencies that require user interaction, complex syscalls, or external hardware states. This framework allows you to provide predictable, scripted responses to these dependencies.

- ### Zero-modifications required

  Target shared objects and DLLs require no modifications or recompilation. The framework performs runtime IAT patching on Windows, and on Linux/macOS it takes advantage of the dynamic loader (`ld.so`, `dyld`) and its dynamic loading order.

- ### Cross-platform

  Provides a unified API that handles the technical differences between all platforms transparently.

- ### Integrity check

  Includes built-in validation to ensure every registered mock is successfully applied across the process, preventing silent test failures.

## Installing

`glib-mock` can be integrated as a Meson subproject or installed system-wide.

- ### As a Meson Subproject
  - Create `subprojects/glib-mock.wrap`:

    ```ini
    [wrap-git]
    url = https://gitlab.gnome.org/otaxhu/glib-mock.git
    revision = main
    depth = 1

    [provide]
    dependency_names = glib-mock
    ```

- ### System-wide Installation
  - Run the following commands to build and install globally:

    ```bash
    meson setup _build
    ninja -C _build
    ninja -C _build install # You may need root privileges
    ```

## Running tests

After you succesfully installed the library, you can run tests by doing the following:

- Add to your `meson.build`:

  ```meson
  libglib_mock_dep = dependency('glib-mock')
  ```

- Then in your meson test:

  ```meson
  test_executable = executable(
    ...,
    dependencies: [libglib_mock_dep],
  )
  test(
    'your-test',
    test_executable,
  )
  ```

## Technical requirements

- ### Dynamic linking only

  `glib-mock` can only intercept functions that are dynamically linked (shared libraries) and dynamically loaded (so no LTO inlines). It will not work with libraries linked statically.

## Platform support

| Feature | Linux | macOS | Windows |
| :-----: | ----- | ----- | ------- |
| Interposition method | Native interposition by exporting symbol at compilation time (user just writes a regular exported function) | **Same as in Linux** | IAT / `GetProcAddress` runtime patching |
| Mock load-time imports | ✅ | ⚠️ Partially supported (Per testing results on GitHub CI, it cannot mock load-time imported functions on recent macOS versions, though per testing results on a personal Mac with macOS Big Sur (11.0), it worked completely fine) | ✅ |
| Mock run-time imports | ✅ (via `dlsym`) | ✅ (via `dlsym`) | ✅ (via `GetProcAddress`, or `dlsym` if using Cygwin) |

  ### Platform specifics
  
  - **Windows**: Run-time dynamically linked functions

    The framework supports mocking [run-time dynamically linked](https://learn.microsoft.com/en-us/windows/win32/dlls/run-time-dynamic-linking) functions on Windows. This ensures that even if a library is loaded at runtime via `LoadLibrary()`, any request for a symbol will transparently return your mock implementation.

  - **macOS**: Requires flat namespace

    This is a requirement for the framework in order to perform interposition without doing any recompilation of libraries (other methods require recompilation and modification to sources in order to perform interpostion). Ultimately, this was the less invasive method to implement the framework in macOS, and may be unstable due to the use of flat-namespaces.
