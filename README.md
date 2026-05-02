# Debugging Tool

A small debugging utility for inspecting behavior.

This project is built with Meson and uses local Wayland protocol definitions from the `protocols/` directory. Generated files and build output are intentionally not committed.

## Features

- Meson/Ninja build setup
- C source code
- Local protocol XML definitions

## Requirements

- Meson
- Ninja
- C compiler
- Wayland development libraries
- `wayland-scanner`

## Dependencies

```bash
install meson ninja-build gcc wayland-devel wayland-protocols-devel
```

## Build

```
mason setup build
meson compile -C build
```

## Run

```
./build/mtags
```


## Project Layout

```
.
├── meson.build
├── protocols/
│   ├── dwl-ipc-unstable-v2.xml
│   ├── ext-foreign-toplevel-list-v1.xml
│   ├── ext-workspace-v1.xml
│   └── wlr-foreign-toplevel-management-unstable-v1.xml
└── src/
    └── main.c
```

**Note**

For this specific project it was necessary to install wlroots 0.19.3, other versions may not work because the wlroots API can change between releases due the nature of this project (MangoWM).
