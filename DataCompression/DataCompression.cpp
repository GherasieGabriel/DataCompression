//Sequential code for Data Compression
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <chrono>
#include <iomanip>

using namespace std;

const int WINDOW_SIZE = 8192; // Size of the sliding window
const int BUFFER_SIZE = 256;  // Size of the look-ahead buffer

struct LZ77Token {
	int offset; // Distance to the start of the match
	int length; // Length of the match
	char next;  // Next character after the match
	LZ77Token(int o = 0, int l = 0, char n = '\0') : offset(o), length(l), next(n) {}
};

// Designed to work on a block of data. By passing start and end indices,
// it enables easy future division of data amongst parallel workers (e.g., MPI nodes).
vector<LZ77Token> compressBlock(const string& input, int start_idx = 0, int end_idx = -1) {
	if (end_idx == -1) end_idx = input.length();

	vector<LZ77Token> output;
	int buffer_pos = start_idx;

	while (buffer_pos < end_idx) {
		int match_length = 0;
		int match_offset = 0;
		char next_char = input[buffer_pos]; // Default to next character without match

		// Define our sliding window boundaries
		int window_start = max(start_idx, buffer_pos - WINDOW_SIZE);

		// Search for the longest match in the window
		for (int i = window_start; i < buffer_pos; i++) {
			int len = 0;
			while (len < BUFFER_SIZE &&
				buffer_pos + len < end_idx &&
				input[i + len] == input[buffer_pos + len]) {
				len++;
			}

			if (len > match_length) {
				match_length = len;
				match_offset = buffer_pos - i;

				// Get the next char after the match, or null if it reaches the end of block
				if (buffer_pos + match_length < end_idx) {
					next_char = input[buffer_pos + match_length];
				}
				else {
					next_char = '\0';
				}
			}
		}

		if (match_length == 0) {
			output.push_back(LZ77Token(0, 0, input[buffer_pos]));
			buffer_pos++;
		}
		else {
			output.push_back(LZ77Token(match_offset, match_length, next_char));
			buffer_pos += match_length + 1;
		}
	}
	return output;
}

int runCompression() {
	ifstream infile("TestCases/test_data.txt", ios::binary);
	string text;

	if (infile.is_open()) {
		text.assign(istreambuf_iterator<char>(infile), istreambuf_iterator<char>());
		infile.close();
	}

	if (text.empty()) {
		cout << "Could not open test_data.txt or it is empty. Using default dataset.\n";
		text = "abracadabraabracadabra";
	}

	cout << "Input size: " << text.size() << " characters\n";

	auto compressionStart = chrono::high_resolution_clock::now();
	vector<LZ77Token> compressed = compressBlock(text);
	auto compressionEnd = chrono::high_resolution_clock::now();
	auto compressionMs = chrono::duration_cast<chrono::milliseconds>(compressionEnd - compressionStart);

	int originalBytes = (int)text.size();
	int compressedBytes = (int)compressed.size() * (sizeof(int) * 2 + sizeof(char)); // Each token has 2 ints and 1 char
	int tokenCount = (int)compressed.size();

	double compressionPercent = 0.0;
	if (originalBytes > 0) {
		compressionPercent = compressedBytes * 100.0 / originalBytes;
	}

	cout << "Compression time: " << compressionMs.count() << " ms\n";
	cout << "Original size: " << originalBytes << " bytes\n";
	cout << "Compressed size: " << compressedBytes << " bytes\n";
	cout << "Compression ratio: " << fixed << setprecision(2) << compressionPercent << "%\n";

	ofstream outfile("compressed.bin", ios::binary);
	if (!outfile.is_open()) {
		cout << "Failed to create compressed.bin\n";
		return 1;
	}

	outfile.write((char*)&tokenCount, sizeof(tokenCount));

	for (const auto& token : compressed) {
		outfile.write((char*)&token.offset, sizeof(token.offset));
		outfile.write((char*)&token.length, sizeof(token.length));
		outfile.write((char*)&token.next, sizeof(token.next));
	}

	outfile.close();

	ofstream txtOutfile("compressed.txt");
	if (!txtOutfile.is_open()) {
		cout << "Failed to create compressed.txt\n";
		return 1;
	}

	for (const auto& token : compressed) {
		int nextValue = (unsigned char)token.next;
		txtOutfile << token.offset << ' ' << token.length << ' ' << nextValue << '\n';
	}
	txtOutfile.close();

	cout << "Compression complete. Tokens: " << compressed.size() << "\n";
	cout << "Output file: compressed.bin\n";
	cout << "Text output file: compressed.txt\n";

	return 0;
}