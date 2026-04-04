cfg_glib_mock_doc = configuration_data()
cfg_glib_mock_doc.set('GLIB_MOCK_VERSION', meson.project_version())

glib_mock_doc = configure_file(
  input: 'glib-mock.toml.in',
  output: 'glib-mock.toml',
  configuration: cfg_glib_mock_doc,
)

gidocgen = find_program('gi-docgen')

custom_target(
  command: [
    gidocgen,
    'generate',
    '-C', glib_mock_doc,
    glib_mock_gir,
    '--output-dir', meson.current_build_dir(),
  ],
  output: 'glib-mock-doc',
  build_by_default: true,
)
