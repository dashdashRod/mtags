project('mtags', 'c', default_options: ['c_std=c11', 'warning_level=2'])

wayland_client = dependency('wayland-client')
wayland_scanner = find_program('wayland-scanner')

# Generate protocol code from XML
protocols = ['ext-workspace-v1', 'wlr-foreign-toplevel-management-unstable-v1', 'dwl-ipc-unstable-v2']
proto_sources = []
proto_headers = []

foreach p : protocols
  xml = 'protocols' / p + '.xml'
  proto_sources += custom_target(p + '-code',
    input: xml,
    output: p + '-protocol.c',
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'])
  proto_headers += custom_target(p + '-header',
    input: xml,
    output: p + '-protocol.h',
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'])
endforeach

executable('mtags',
  ['src/main.c'] + proto_sources + proto_headers,
  dependencies: [wayland_client])
