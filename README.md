# API Kit

A cross-platform HTTP client application with a graphical user interface, similar to Postman.

## Features

- **HTTP Client**: Support for GET, POST, PUT, DELETE, PATCH requests
- **GUI Interface**: Modern graphical interface using Nuklear
- **Collections**: Organize requests into workspaces and collections
- **History**: Track and replay previous requests
- **Settings**: Configurable data paths, themes, and keyboard shortcuts
- **File Format**: Uses HTTP file format compatible with VS Code REST Client

## Project Structure

```
apikit/
├── src/                    # Source files
│   ├── main.c             # Main application
│   ├── http_client.c      # HTTP client implementation
│   └── http_parser.c      # HTTP file format parser
├── include/               # Header files
│   ├── http_client.h      # HTTP client interface
│   └── http_parser.h      # HTTP parser interface
├── libs/                  # Third-party libraries
│   ├── nuklear/          # Nuklear GUI library
│   └── tomlc99/          # TOML configuration parser
├── data/                 # Application data
├── docs/                 # Documentation
├── examples/             # Usage examples
└── build/                # Build output
```

## Building

### Prerequisites

- CMake 3.10 or higher
- C99 compatible compiler
- GLFW3
- cURL
- OpenGL

### macOS (Homebrew)

```bash
brew install cmake glfw curl
```

### Build Commands

```bash
# Configure
cmake -B build

# Build
cmake --build build

# Run
./build/bin/apikit
```

## Configuration

Settings are stored in `config.toml`:

```toml
# API Kit Configuration

[general]
data_folder = "data"
theme = 0

[keybindings]
enabled = true
ctrl_b_enabled = true
ctrl_f_enabled = true
delete_key_enabled = true
```

## Usage

### Keyboard Shortcuts

- `Ctrl+B`: Toggle sidebar
- `Ctrl+F`: Focus search
- `Delete`: Delete selected item
- `Ctrl+A/C/V`: Text editing shortcuts

### Workspace Management

1. Create workspaces to organize your requests
2. Add collections within workspaces
3. Save requests to collections for reuse
4. All data stored in HTTP file format

## Development

### Adding Features

1. Source files go in `src/`
2. Headers go in `include/`
3. Third-party libraries go in `libs/`
4. Update CMakeLists.txt as needed

### Dependencies

- **Nuklear**: Immediate mode GUI library
- **GLFW**: Window and input management
- **cURL**: HTTP client functionality
- **tomlc99**: TOML configuration parsing
