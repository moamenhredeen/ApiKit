# Development Guide

## Project Architecture

API Kit follows a modular architecture with clear separation between components:

### Core Components

1. **HTTP Client** (`src/http_client.c`, `include/http_client.h`)
   - Handles HTTP requests using cURL
   - Supports all standard HTTP methods
   - Manages headers and request/response bodies

2. **HTTP Parser** (`src/http_parser.c`, `include/http_parser.h`)
   - Parses HTTP file format
   - Compatible with VS Code REST Client format
   - Handles collections and workspaces

3. **Main Application** (`src/main.c`)
   - GUI implementation using Nuklear
   - Application state management
   - Event handling and user interaction

### Third-Party Libraries

- **Nuklear**: Immediate mode GUI framework
- **GLFW**: Window management and OpenGL context
- **cURL**: HTTP client functionality
- **tomlc99**: TOML configuration parsing

## Building and Development

### Debug Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Adding New Features

1. **New HTTP Functionality**: Extend `http_client.c`
2. **Parser Improvements**: Modify `http_parser.c`
3. **GUI Features**: Update `main.c`
4. **Configuration**: Add to TOML structure

### Code Style

- Use consistent indentation (4 spaces)
- Follow C99 standards
- Add comments for complex logic
- Use descriptive variable names

### Memory Management

- Always free allocated memory
- Use proper error handling
- Check return values from library functions

## Testing

### Manual Testing

1. Build in debug mode
2. Test with various HTTP endpoints
3. Verify file format compatibility
4. Test keyboard shortcuts and UI

### Common Test Cases

- GET/POST/PUT/DELETE requests
- JSON and form data bodies
- Custom headers
- Authentication
- Error handling

## Contributing

1. Follow the existing code style
2. Test thoroughly
3. Update documentation
4. Add examples for new features
