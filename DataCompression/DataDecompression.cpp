#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <cstdint>

using namespace std;

struct LZ77Token {
	bool isMatch;
	uint16_t offset;
	uint16_t length;
	char literal;
	LZ77Token(bool match = false, uint16_t o = 0, uint16_t l = 0, char ch = '\0')
		: isMatch(match), offset(o), length(l), literal(ch) {}
};

string decompressBlock(const vector<LZ77Token>& tokens) {
	string output;
	size_t estimatedSize = 0;
	for (const auto& token : tokens) {
		estimatedSize += token.isMatch ? token.length : 1;
	}
	output.reserve(estimatedSize);

	for (const auto& token : tokens) {
		if (token.isMatch) {
			if (token.offset == 0 || token.length == 0 || token.offset > output.size()) {
				return "";
			}

			int start = (int)output.size() - token.offset;
			for (int i = 0; i < token.length; i++) {
				output += output[start + i];
			}
		}
		else {
			output += token.literal;
		}
	}
	return output;
}

int runDecompression() {
	ifstream infile("compressed.bin", ios::binary);
	if (!infile.is_open()) {
		cout << "Could not open compressed.bin\n";
		return 1;
	}

	infile.seekg(0, ios::end);
	streamsize inputSize = infile.tellg();
	infile.seekg(0, ios::beg);
	cout << "Input size: " << inputSize << " characters\n";

	int tokenCount = 0;
	infile.read((char*)&tokenCount, sizeof(tokenCount));
	if (!infile || tokenCount < 0) {
		cout << "Invalid compressed.bin format\n";
		return 1;
	}

	vector<LZ77Token> tokens;
	tokens.reserve(tokenCount);

	for (int i = 0; i < tokenCount; i += 8) {
		uint8_t flags = 0;
		infile.read((char*)&flags, sizeof(flags));
		if (!infile) {
			cout << "Unexpected end of compressed.bin\n";
			return 1;
		}

		int groupEnd = min(i + 8, tokenCount);
		for (int j = i; j < groupEnd; ++j) {
			bool isMatch = ((flags >> (j - i)) & 1u) != 0;
			if (isMatch) {
				uint16_t offset = 0;
				uint16_t length = 0;
				infile.read((char*)&offset, sizeof(offset));
				infile.read((char*)&length, sizeof(length));
				if (!infile) {
					cout << "Unexpected end of compressed.bin\n";
					return 1;
				}
				tokens.emplace_back(true, offset, length, '\0');
			}
			else {
				char literal = '\0';
				infile.read((char*)&literal, sizeof(literal));
				if (!infile) {
					cout << "Unexpected end of compressed.bin\n";
					return 1;
				}
				tokens.emplace_back(false, 0, 0, literal);
			}
		}
	}

	infile.close();

	auto decompressionStart = chrono::high_resolution_clock::now();
	string decompressed = decompressBlock(tokens);
	auto decompressionEnd = chrono::high_resolution_clock::now();
	auto decompressionMs = chrono::duration_cast<chrono::milliseconds>(decompressionEnd - decompressionStart);

	if (decompressed.empty() && !tokens.empty()) {
		cout << "Invalid compressed token stream\n";
		return 1;
	}

	cout << "Decompression time: " << decompressionMs.count() << " ms\n";

	ofstream outfile("decompressed.txt", ios::binary);
	if (!outfile.is_open()) {
		cout << "Failed to create decompressed.txt\n";
		return 1;
	}

	outfile << decompressed;
	outfile.close();

	cout << "Decompression complete. Output file: decompressed.txt\n";
	return 0;
}
