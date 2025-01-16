#include "parseCommandLine.h"
#include "graphIO.h"
#include "parallel.h"

int parallel_main(int argc, char* argv[]) {
    commandLine P(argc, argv, "[-s] <input results file> <output results file>");
    char* iFile1 = P.getArgument(0);
    char* iFile2 = P.getArgument(1);

    std::ifstream file1(iFile1);
    std::ifstream file2(iFile2);

    if (!file1.is_open() || !file2.is_open()) {
        std::cerr << "Error: Unable to open one or both input files." << std::endl;
        return 1;
    }

    std::string line1, line2;
    int lineNumber = 0;
    int differences = 0;

    while (std::getline(file1, line1) && std::getline(file2, line2)) {
        lineNumber++;

        std::istringstream iss1(line1);
        std::istringstream iss2(line2);

        int n1, result1;
        int n2, result2;

        if (!(iss1 >> n1 >> result1) || !(iss2 >> n2 >> result2)) {
            std::cerr << "Error: Malformed line at line number " << lineNumber << std::endl;
            return 1;
        }

        if (n1 != n2 || result1 != result2) {
            differences++;
            std::cout << "Difference found at line " << lineNumber << ": "
                      << "File1(" << n1 << ", " << result1 << ") vs File2(" << n2 << ", " << result2 << ")" << std::endl;
        }
    }

    if (differences == 0) {
        std::cout << "The files are identical." << std::endl;
    } else {
        std::cout << "Total differences: " << differences << std::endl;
    }

    file1.close();
    file2.close();

    return 0;
}