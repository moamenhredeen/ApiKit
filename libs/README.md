# Third-Party Libraries

This directory contains external libraries used by API Kit.

## Nuklear (`nuklear/`)

- **Purpose**: Immediate mode GUI library
- **Version**: Latest from official repository
- **License**: Public Domain / MIT
- **Files**: 
  - `nuklear.h` - Main header
  - `nuklear_glfw_gl3.h` - GLFW/OpenGL3 backend

## tomlc99 (`tomlc99/`)

- **Purpose**: TOML configuration file parser
- **Version**: Latest from official repository
- **License**: MIT
- **Files**:
  - `toml.h` - Header file
  - `toml.c` - Implementation

## Updating Libraries

To update these libraries:

1. **Nuklear**: Download latest from https://github.com/Immediate-Mode-UI/Nuklear
2. **tomlc99**: Download latest from https://github.com/cktan/tomlc99

Replace the files in their respective directories and test the build.

## License Information

Each library maintains its own license. Please refer to their respective repositories for detailed license information.
