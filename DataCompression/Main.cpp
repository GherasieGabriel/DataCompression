#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <iterator>
#include <algorithm>

using namespace std;

int runCompression();
int runDecompression();

string readFileContents(const string& path) {
	ifstream infile(path, ios::binary);
	if (!infile.is_open()) {
		return "";
	}

	return string(istreambuf_iterator<char>(infile), istreambuf_iterator<char>());
}

string readSelectedCasePath() {
	ifstream infile("last_case.txt");
	if (!infile.is_open()) {
		return "";
	}

	string selectedPath;
	getline(infile, selectedPath);
	return selectedPath;
}

void recordDataLossReport(const string& original, const string& decompressed) {
	int originalSize = (int)original.size();
	int decompressedSize = (int)decompressed.size();
	int minSize = min(originalSize, decompressedSize);
	int mismatchedBytes = 0;

	for (int i = 0; i < minSize; ++i) {
		if (original[i] != decompressed[i]) {
			++mismatchedBytes;
		}
	}

	int sizeDifference = 0;
	if (originalSize > decompressedSize) {
		sizeDifference = originalSize - decompressedSize;
	}
	else {
		sizeDifference = decompressedSize - originalSize;
	}

	int totalDifferentBytes = mismatchedBytes + sizeDifference;
	double lossPercent = 0.0;
	if (originalSize > 0) {
		lossPercent = (double)totalDifferentBytes * 100.0 / (double)originalSize;
	}
	else if (decompressedSize > 0) {
		lossPercent = 100.0;
	}
	if (totalDifferentBytes == 0)
		cout << "OK (0.00% loss)\n";

	else
		cout << "LOSS (" << fixed << setprecision(2) << lossPercent << "% loss)\n";

}

int main() {
	cout << "Choose operation:\n";
	cout << "1. Compress\n";
	cout << "2. Decompress\n";
	cout << "3. Compress then Decompress\n";
	cout << "Enter choice: ";

	int choice = 0;
	cin >> choice;

	if (choice == 1) {
		return runCompression();
	}
	if (choice == 2) {
		return runDecompression();
	}
	if (choice == 3) {
		int compressionResult = runCompression();
		if (compressionResult != 0) {
			return compressionResult;
		}

		int decompressionResult = runDecompression();
		if (decompressionResult != 0) {
			return decompressionResult;
		}

		string selectedCasePath = readSelectedCasePath();
		string original = selectedCasePath.empty() ? "" : readFileContents(selectedCasePath);
		if (original.empty()) {
			original = "abracadabraabracadabra";
		}

		string decompressed = readFileContents("decompressed.txt");
		if (decompressed.empty()) {
			cout << "Could not read decompressed.txt for integrity check\n";
			return 1;
		}

		recordDataLossReport(original, decompressed);
		return 0;
	}

	cout << "Invalid choice\n";
	return 1;
}
