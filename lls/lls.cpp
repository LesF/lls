#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <ctime>
#include <windows.h>

namespace fs = std::filesystem;

// Struct to hold the options for the ls command
struct LsOptions {
    bool longListing = false;  // -l: Long listing format
    bool directoriesOnly = false;  // -d: List directories only
    bool sortByTime = false;  // -t: Sort by modification time
    bool sortBySize = false;  // -s: Sort by file size
    bool oneColumn = false;  // -1: One column output
    bool multiColumn = false;  // -C: Multi-column output
    std::string directory = ".";  // Default directory is the current directory
};

// Display usage information
void DisplayUsage() {
    std::cout << "Usage: lls [options] [directory]\n"
        << "Options:\n"
        << "  -l        Long listing format\n"
        << "  -d        List directories only\n"
        << "  -t        Sort by modification time\n"
        << "  -s        Sort by file size\n"
        << "  -1        One column output\n"
        << "  -C        Multi-column output\n";
}

// Function to check if a string is a valid directory
bool IsValidDirectory(const std::string& dir) {
    return fs::exists(dir) && fs::is_directory(dir);
}

// Function to parse command-line arguments and populate LsOptions
void ParseArguments(int argc, char* argv[], LsOptions& options) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-l") {
            options.longListing = true;
        }
        else if (arg == "-d") {
            options.directoriesOnly = true;
        }
        else if (arg == "-t") {
            options.sortByTime = true;
        }
        else if (arg == "-s") {
            options.sortBySize = true;
        }
        else if (arg == "-1") {
            options.oneColumn = true;
        }
        else if (arg == "-C") {
            options.multiColumn = true;
        }
        else if (arg == "--help" || arg == "-h" || arg == "-?") {
            DisplayUsage();
            exit(0);
        }
        else if (arg == "--version") {
            std::cout << "lls version 1.0.0\n";
            exit(0);
        }
        else {
            // Assume any non-option argument is the directory
            if (arg[0] == '-') {
                std::cerr << "Unknown option: " << arg << std::endl;
                DisplayUsage();
                continue;
            }
            // Check if the argument is a valid directory
            if (!IsValidDirectory(arg)) {
                std::cerr << "Invalid directory: " << arg << std::endl;
                DisplayUsage();
                continue;
            }
            options.directory = arg;
        }
    }
}

// Function to get the width of the console
int GetConsoleWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 80; // Default to 80 columns if unable to determine
}

// Function to list files and directories based on LsOptions
void ListFiles(const LsOptions& options) {
    try {
        // Get the directory iterator
        fs::directory_iterator dirIter(options.directory);
        std::vector<fs::directory_entry> entries;

        // Collect entries based on options
        for (const auto& entry : dirIter) {
            if (options.directoriesOnly && !entry.is_directory()) {
                continue; // Skip non-directories if -d is specified
            }
            entries.push_back(entry);
        }

        // Sort entries based on options
        if (options.sortByTime) {
            std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
                return fs::last_write_time(a) > fs::last_write_time(b);
                });
        }
        else if (options.sortBySize) {
            std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
                return fs::file_size(a) > fs::file_size(b);
                });
        }

        if (options.multiColumn) {
            int consoleWidth = GetConsoleWidth();
            int maxFilenameLength = 0;
            // Determine the maximum filename length
            for (const auto& entry : entries) {
                int filenameLength = static_cast<int>(entry.path().filename().string().length());
                if (filenameLength > maxFilenameLength) {
                    maxFilenameLength = filenameLength;
                }
            }
            // Calculate the number of columns
            int columns = consoleWidth / (maxFilenameLength + 2); // Add 2 for spacing
            if (columns < 1) columns = 1;

            // Print filenames in columns
            int count = 0;
            for (const auto& entry : entries) {
                std::cout << std::setw(maxFilenameLength) << std::left << entry.path().filename().string() << "  ";
                if (++count % columns == 0) {
                    std::cout << std::endl;
                }
            }
            if (count % columns != 0) {
                std::cout << std::endl; // Add a newline if the last row is incomplete
            }
            return; // Exit after multi-column output
        }

        // Display entries
        for (const auto& entry : entries) {
            if (options.longListing) {
                // Long listing format
                auto ftime = fs::last_write_time(entry);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);

                // Use localtime_s for thread-safe conversion (without std:: prefix)
                std::tm timeInfo;
                if (localtime_s(&timeInfo, &cftime) == 0) {
                    std::cout << (entry.is_directory() ? "d" : "-")
                        << std::setw(10) << fs::file_size(entry) << " "
                        << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S") << " "
                        << entry.path().filename().string() << std::endl;
                }
                else {
                    std::cerr << "Error converting time for file: " << entry.path().filename().string() << std::endl;
                }
            }
            else {
                // Simple listing format
                std::cout << entry.path().filename().string() << (options.oneColumn ? "\n" : "  ");
            }
        }

        if (!options.oneColumn) {
            std::cout << std::endl; // Add a newline for multi-column output
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    LsOptions options;

    // Parse command-line arguments
    ParseArguments(argc, argv, options);

    // Debug output to verify options
    //std::cout << "Options parsed:" << std::endl;
    //std::cout << "Directory: " << options.directory << std::endl;
    //std::cout << "Long listing: " << (options.longListing ? "true" : "false") << std::endl;
    //std::cout << "Directories only: " << (options.directoriesOnly ? "true" : "false") << std::endl;
    //std::cout << "Sort by time: " << (options.sortByTime ? "true" : "false") << std::endl;
    //std::cout << "Sort by size: " << (options.sortBySize ? "true" : "false") << std::endl;
    //std::cout << "One column: " << (options.oneColumn ? "true" : "false") << std::endl;
    //std::cout << "Multi-column: " << (options.multiColumn ? "true" : "false") << std::endl;

    ListFiles(options);

    return 0;
}
