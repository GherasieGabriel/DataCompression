#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>

using namespace std;

struct LZ77Token {
	int offset;
	int length;
	char next;
	LZ77Token(int o = 0, int l = 0, char n = '\0') : offset(o), length(l), next(n) {}
};

string decompressBlock(const vector<LZ77Token>& tokens) {
	string output;
	for (const auto& token : tokens) {
		if (token.offset > 0 && token.length > 0) {
			int start = static_cast<int>(output.size()) - token.offset;
			for (int i = 0; i < token.length; i++) {
				output += output[start + i];
			}
		}
		if (token.next != '\0') {
			output += token.next;
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

	for (int i = 0; i < tokenCount; i++) {
		LZ77Token token;
		infile.read((char*)&token.offset, sizeof(token.offset));
		infile.read((char*)&token.length, sizeof(token.length));
		infile.read((char*)&token.next, sizeof(token.next));
		if (!infile) {
			cout << "Unexpected end of compressed.bin\n";
			return 1;
		}
		tokens.push_back(token);
	}

	infile.close();

	auto decompressionStart = chrono::high_resolution_clock::now();
	string decompressed = decompressBlock(tokens);
	auto decompressionEnd = chrono::high_resolution_clock::now();
	auto decompressionMs = chrono::duration_cast<chrono::milliseconds>(decompressionEnd - decompressionStart);

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
