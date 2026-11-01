cat > docs/mainpage.dox <<'EOF'
/**
 * @mainpage TIVIDY Documentation
 *
 * @section intro Introduction
 *
 * TIVIDY is a terminal-based workflow management system
 * developed as part of COS214.
 *
 * @section purpose Purpose
 *
 * The system aims to help users manage workflows and their
 * associated tasks through a command-line interface.
 *
 * @section structure Project Structure
 *
 * - include/: Header files and class declarations.
 * - src/: Source files and implementations.
 * - tests/: Automated tests.
 * - docs/: Project documentation.
 *
 * @section build Build
 *
 * The project uses C++11.
 *
 * @section documentation Generating Documentation
 *
 * Run the following command from the project root:
 *
 * @code
 * doxygen Doxyfile
 * @endcode
 *
 * Open docs/generated/html/index.html in a browser to view
 * the generated documentation.
 */
EOF