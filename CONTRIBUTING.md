# Contributing to ESP32-S3 WiFi & USB Printer Manager

Thank you for your interest in contributing! This document provides guidelines and instructions for contributing.

## Code of Conduct

- Be respectful and inclusive
- Provide constructive feedback
- Focus on the work, not the person
- Help others learn and improve

## How to Contribute

### Reporting Bugs

1. **Check Existing Issues** - Search for duplicate reports
2. **Create Detailed Report** with:
   - Clear, descriptive title
   - Steps to reproduce
   - Expected behavior
   - Actual behavior
   - Hardware and software information
   - Relevant logs or screenshots

### Suggesting Enhancements

1. **Check Existing Issues** - Look for similar suggestions
2. **Create Enhancement Proposal** with:
   - Clear, descriptive title
   - Detailed description of the enhancement
   - Use cases and benefits
   - Possible implementation approach
   - Alternative solutions considered

### Submitting Pull Requests

1. **Fork the Repository**
   ```bash
   git clone https://github.com/your-username/esp32-printer-manager.git
   cd esp32-printer-manager
   ```

2. **Create Feature Branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make Your Changes**
   - Follow code style guidelines (see below)
   - Write clear commit messages
   - Add/update documentation as needed
   - Test your changes thoroughly

4. **Commit Your Changes**
   ```bash
   git add .
   git commit -m "Descriptive commit message"
   ```

5. **Push to Your Fork**
   ```bash
   git push origin feature/your-feature-name
   ```

6. **Open Pull Request**
   - Provide clear title and description
   - Reference any related issues (#123)
   - Include before/after screenshots if applicable
   - Ensure tests pass

## Code Style Guidelines

### C/C++ Code

- **Naming Conventions**
  - Variables: `snake_case` (e.g., `my_variable`)
  - Functions: `snake_case` (e.g., `my_function()`)
  - Macros: `UPPER_SNAKE_CASE` (e.g., `MY_MACRO`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MY_CONSTANT`)
  - Types: `PascalCase` (e.g., `MyType`)

- **Formatting**
  - 4 spaces for indentation (no tabs)
  - Line length: max 120 characters
  - Braces: Allman style (opening brace on new line)
  - Spaces around operators

- **Comments**
  - Use Doxygen-style comments for functions:
    ```c
    /**
     * @brief Brief description
     * 
     * @param param1 Description of param1
     * @param param2 Description of param2
     * @return Description of return value
     */
    esp_err_t my_function(int param1, const char *param2);
    ```

- **Example**
  ```c
  esp_err_t printer_manager_write_text(const char *text)
  {
      if (!g_initialized || !g_printer_mutex) {
          return ESP_ERR_INVALID_STATE;
      }

      if (!text) {
          return ESP_ERR_INVALID_ARG;
      }

      if (xSemaphoreTake(g_printer_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
          return ESP_ERR_TIMEOUT;
      }

      esp_err_t err = ESP_ERR_INVALID_STATE;
      if (g_printer) {
          err = escpos_write_text(g_printer, text);
      }

      xSemaphoreGive(g_printer_mutex);
      return err;
  }
  ```

### HTML/JavaScript

- **Formatting**
  - 2 spaces for indentation
  - Semicolons required for statements
  - Use camelCase for variables and functions

- **Comments**
  - Use JSDoc style for functions
  - Comment complex logic
  - Keep comments updated with code changes

- **Example**
  ```javascript
  /**
   * Send text to printer
   * @param {string} text - Text to print (max 256 chars)
   * @returns {Promise<Object>} Response from server
   */
  function printText(text)
  {
    if (!text || text.trim().length === 0) {
      showMessage('Please enter text to print', 'error');
      return;
    }

    fetch(`${API_BASE}/printer/print`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `text=${encodeURIComponent(text)}`
    })
    .then(r => r.json())
    .then(data => {
      if (data.error) {
        showMessage('Error: ' + data.error, 'error');
      } else {
        showMessage('Text sent to printer', 'success');
      }
    })
    .catch(err => showMessage('Failed: ' + err.message, 'error'));
  }
  ```

## Building and Testing

### Local Build

```bash
# Set ESP-IDF environment
export IDF_PATH=~/esp/esp-idf
source $IDF_PATH/export.sh

# Build project
idf.py set-target esp32s3
idf.py build

# Clean build
idf.py fullclean && idf.py build
```

### Testing Checklist

Before submitting a PR, verify:

- [ ] Code compiles without warnings
- [ ] All existing functionality still works
- [ ] New code follows style guidelines
- [ ] Documentation is updated
- [ ] HTML UI renders correctly
- [ ] All endpoints return correct JSON
- [ ] Error handling works properly
- [ ] No memory leaks (check with `idf.py size-monitor`)

### Testing on Hardware

1. **Flash to ESP32-S3**
   ```bash
   idf.py -p COM3 flash monitor
   ```

2. **Verify Functionality**
   - Check WiFi AP is broadcasting
   - Verify web interface loads
   - Test printer endpoints with USB printer
   - Check for any error messages in logs

3. **Monitor Performance**
   ```bash
   idf.py monitor --baudrate 115200 -p COM3
   ```

## Documentation

- Update README.md for user-facing changes
- Add inline code comments for complex logic
- Use Doxygen-style comments in headers
- Document API changes in PR description
- Include examples for new features

## Commit Message Guidelines

- Use clear, descriptive titles (50 chars max)
- Provide detailed description if needed (wrap at 72 chars)
- Reference issues: "Fixes #123" or "Related to #456"
- Use present tense: "Add feature" not "Added feature"

Examples:
```
Add USB printer status endpoint

- Implement /printer/status GET endpoint
- Return JSON with connection status
- Add tests for status checks
- Update API documentation

Fixes #42
```

## Review Process

1. **Automated Checks**
   - Code compilation
   - Style validation
   - Test execution

2. **Manual Review**
   - Code quality assessment
   - Design review
   - Documentation review
   - User impact analysis

3. **Approval**
   - Maintainer will review and provide feedback
   - Address requested changes
   - Request re-review if needed

## Development Workflow Example

```bash
# 1. Update local main branch
git checkout main
git pull origin main

# 2. Create feature branch
git checkout -b feature/add-status-endpoint

# 3. Make changes
# Edit files...
nano main/http_server.c

# 4. Build and test locally
idf.py build
idf.py -p COM3 flash monitor

# 5. Commit changes
git add main/http_server.c
git commit -m "Add status endpoint

- Implement GET /status endpoint
- Return JSON with system status
- Add response validation"

# 6. Push to fork
git push origin feature/add-status-endpoint

# 7. Create pull request on GitHub
# Open browser: https://github.com/your-username/esp32-printer-manager/pulls
```

## Useful Resources

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)
- [Doxygen Documentation](http://www.doxygen.nl/)
- [Git Documentation](https://git-scm.com/doc)
- [GitHub Help](https://help.github.com/)

## Questions?

- Check existing issues and discussions
- Create a new discussion for questions
- Contact maintainers directly if needed

---

Thank you for contributing to make this project better! 🎉
