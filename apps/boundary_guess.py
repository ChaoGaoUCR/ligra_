import sys
import numpy as np

def read_file(file_path):
    """Reads a file and returns its lines as a list of floats."""
    with open(file_path, 'r') as file:
        return [float(line.strip()) for line in file.readlines()]

def parse_boundaries(file_path):
    """Reads a boundary file and returns a list of (a, b) tuples."""
    with open(file_path, 'r') as file:
        boundaries = []
        for line in file.readlines():
            a, b = map(float, line.strip().split())
            boundaries.append((a, b))
        return boundaries

def check_overlap(boundary1, boundary2):
    """Check if two boundaries overlap."""
    a1, b1 = boundary1
    a2, b2 = boundary2
    return max(a1, a2) <= min(b1, b2)

def main(result_file1, result_file2, boundary_file1, boundary_file2):
    # Read files
    results1 = read_file(result_file1)
    results2 = read_file(result_file2)
    boundaries1 = parse_boundaries(boundary_file1)
    boundaries2 = parse_boundaries(boundary_file2)

    # Ensure files have the same number of entries
    assert len(results1) == len(results2) == len(boundaries1) == len(boundaries2), \
        "Files must have the same number of entries."

    correct_predictions = 0
    total_entries = len(results1)
    result_changed_num = 0
    for i in range(total_entries):
        overlap = check_overlap(boundaries1[i], boundaries2[i])
        result_changed = results1[i] != results2[i]
        if result_changed:
            result_changed_num += 1
        # Predict result will change if boundaries do not overlap
        prediction = not overlap

        if prediction == result_changed:
            correct_predictions += 1

    accuracy = correct_predictions / total_entries
    print(f"Correct result_changed: {result_changed_num}")
    print(f"Prediction accuracy: {accuracy:.2%}")

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("Usage: python script.py <result_file1> <result_file2> <boundary_file1> <boundary_file2>")
        sys.exit(1)

    result_file1 = sys.argv[1]
    result_file2 = sys.argv[2]
    boundary_file1 = sys.argv[3]
    boundary_file2 = sys.argv[4]

    main(result_file1, result_file2, boundary_file1, boundary_file2)
