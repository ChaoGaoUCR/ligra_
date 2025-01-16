import argparse

def read_file(file_path):
    data = {}
    with open(file_path, 'r') as file:
        for line in file:
            n, result = line.strip().split(' ')
            data[int(n)] = float(result)
    return data

def compare_results(file1, file2):
    data1 = read_file(file1)
    data2 = read_file(file2)

    different_count = 0
    num = 0
    common_keys = set(data1.keys()) & set(data2.keys())
    for key in common_keys:
        num += 1
        if data1[key] != data2[key]:
            different_count += 1

    accuracy = 1 - (different_count / num) if num > 0 else 0
    print(f"Number of accuracy results: {accuracy:.3f} (1 - {different_count}/{num})")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compare two result files.")
    parser.add_argument("file1", type=str, help="Path to the first file")
    parser.add_argument("file2", type=str, help="Path to the second file")
    args = parser.parse_args()

    compare_results(args.file1, args.file2)
