//Sequential code for Data Compression
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <array>
#include <cstdint>
#include <limits>

using namespace std;
namespace fs = std::filesystem;

const int WINDOW_SIZE = 16384;
const int BUFFER_SIZE = 512;
const int MAX_CANDIDATE_MATCHES = 64;
const int MIN_MATCH_LENGTH = 3;
const char* PRIMARY_CASES_DIRECTORY = "TestCases";
const char* FALLBACK_CASES_DIRECTORY = "DataCompression/TestCases";
const char* LAST_CASE_METADATA_PATH = "last_case.txt";

struct LZ77Token {
	bool isMatch;
	uint16_t offset;
	uint16_t length;
	char literal;
	LZ77Token(bool match = false, uint16_t o = 0, uint16_t l = 0, char ch = '\0')
		: isMatch(match), offset(o), length(l), literal(ch) {}
};

struct TestCase {
	string name;
	string path;
};

string getCasesDirectory() {
	if (fs::exists(PRIMARY_CASES_DIRECTORY)) {
		return PRIMARY_CASES_DIRECTORY;
	}
	if (fs::exists(FALLBACK_CASES_DIRECTORY)) {
		return FALLBACK_CASES_DIRECTORY;
	}
	return "";
}

vector<TestCase> findTestCases() {
	vector<TestCase> cases;
	string casesDirectory = getCasesDirectory();
	if (casesDirectory.empty()) {
		return cases;
	}

	for (const auto& entry : fs::directory_iterator(casesDirectory)) {
		if (!entry.is_regular_file() || entry.path().extension() != ".txt") {
			continue;
		}

		string filename = entry.path().filename().string();
		if (filename == "test_cases.txt") {
			continue;
		}

		cases.push_back({ entry.path().stem().string(), entry.path().generic_string() });
	}

	sort(cases.begin(), cases.end(), [](const TestCase& a, const TestCase& b) {
		return a.name < b.name;
	});

	return cases;
}

string readLastCasePath() {
	ifstream infile(LAST_CASE_METADATA_PATH);
	if (!infile.is_open()) {
		return "";
	}

	string path;
	getline(infile, path);
	return path;
}

void writeLastCaseMetadata(const TestCase& testCase) {
	ofstream outfile(LAST_CASE_METADATA_PATH);
	if (!outfile.is_open()) {
		return;
	}

	outfile << testCase.path << '\n' << testCase.name << '\n';
}

int findDefaultCaseIndex(const vector<TestCase>& cases) {
	string lastPath = readLastCasePath();
	for (int i = 0; i < (int)cases.size(); ++i) {
		if (cases[i].path == lastPath) {
			return i;
		}
	}
	return 0;
}

int chooseCaseIndex(const vector<TestCase>& cases) {
	if (cases.empty()) {
		return -1;
	}

	int defaultIndex = findDefaultCaseIndex(cases);

	cout << "Available test cases:\n";
	for (int i = 0; i < (int)cases.size(); ++i) {
		cout << setw(2) << (i + 1) << ". " << cases[i].name;
		if (i == defaultIndex) {
			cout << "  [current]";
		}
		cout << '\n';
	}

	while (true) {
		cout << "Enter case number: ";
		string selection;
		getline(cin, selection);

		try {
			int selected = stoi(selection);
			if (selected >= 1 && selected <= (int)cases.size()) {
				return selected - 1;
			}
		}
		catch (...) {
		}

		cout << "Invalid selection. Please enter a number between 1 and " << cases.size() << ".\n";
	}
}

// Designed to work on a block of data. By passing start and end indices,
// it enables easy future division of data amongst parallel workers (e.g., MPI nodes).
vector<LZ77Token> compressBlock(const string& input, int start_idx = 0, int end_idx = -1) {
	if (end_idx == -1) end_idx = (int)input.length();

	vector<LZ77Token> output;
	output.reserve(max(1, (end_idx - start_idx) / 2));

	array<vector<int>, 256> positionsByByte;
	array<size_t, 256> activeStart{};

	int buffer_pos = start_idx;
	while (buffer_pos < end_idx) {
		int bestLength = 0;
		int bestOffset = 0;

		unsigned char currentKey = static_cast<unsigned char>(input[buffer_pos]);
		auto& positions = positionsByByte[currentKey];
		size_t& startIndex = activeStart[currentKey];

		int windowStart = max(start_idx, buffer_pos - WINDOW_SIZE);
		while (startIndex < positions.size() && positions[startIndex] < windowStart) {
			++startIndex;
		}

		int candidatesChecked = 0;
		for (size_t idx = positions.size(); idx > startIndex && candidatesChecked < MAX_CANDIDATE_MATCHES; --idx, ++candidatesChecked) {
			int i = positions[idx - 1];
			int len = 0;
			while (len < BUFFER_SIZE &&
				buffer_pos + len < end_idx &&
				input[i + len] == input[buffer_pos + len]) {
				++len;
			}

			if (len > bestLength) {
				bestLength = len;
				bestOffset = buffer_pos - i;
				if (bestLength == BUFFER_SIZE) {
					break;
				}
			}
		}

		int advance = 1;
		if (bestLength >= MIN_MATCH_LENGTH) {
			output.emplace_back(true, (uint16_t)bestOffset, (uint16_t)bestLength, '\0');
			advance = bestLength;
		}
		else {
			output.emplace_back(false, 0, 0, input[buffer_pos]);
		}

		int consumedEnd = min(end_idx, buffer_pos + advance);
		for (int pos = buffer_pos; pos < consumedEnd; ++pos) {
			unsigned char key = static_cast<unsigned char>(input[pos]);
			positionsByByte[key].push_back(pos);
		}

		buffer_pos += advance;
	}
	return output;
}

int runCompression() {
	vector<TestCase> cases = findTestCases();
	string text;
	TestCase selectedCase{ "default", "" };

	cin.ignore(numeric_limits<streamsize>::max(), '\n');

	if (!cases.empty()) {
		int caseIndex = chooseCaseIndex(cases);
		if (caseIndex >= 0) {
			selectedCase = cases[caseIndex];
			ifstream infile(selectedCase.path, ios::binary);
			if (infile.is_open()) {
				text.assign(istreambuf_iterator<char>(infile), istreambuf_iterator<char>());
				infile.close();
			}
		}
	}

	if (text.empty()) {
		cout << "Could not load selected test case. Using default dataset.\n";
		text = "abracadabraabracadabra";
		selectedCase = { "default_fallback", "" };
	}

	cout << "Selected case: " << selectedCase.name;
	if (!selectedCase.path.empty()) {
		cout << " (" << selectedCase.path << ")";
	}
	cout << "\n";
	cout << "Input size: " << text.size() << " characters\n";

	auto compressionStart = chrono::high_resolution_clock::now();
	vector<LZ77Token> compressed = compressBlock(text);
	auto compressionEnd = chrono::high_resolution_clock::now();
	auto compressionMs = chrono::duration_cast<chrono::milliseconds>(compressionEnd - compressionStart);

	int originalBytes = (int)text.size();
	int tokenCount = (int)compressed.size();
	int compressedBytes = (int)sizeof(tokenCount);
	compressedBytes += (tokenCount + 7) / 8;
	for (const auto& token : compressed) {
		compressedBytes += token.isMatch ? (int)(sizeof(uint16_t) * 2) : (int)sizeof(char);
	}

	cout << "Compression time: " << compressionMs.count() << " ms\n";
	cout << "Original size: " << originalBytes << " bytes\n";
	cout << "Compressed size: " << compressedBytes << " bytes\n";
	cout << "Compression ratio: " << fixed << setprecision(2) << (compressedBytes * 100.0 / originalBytes) << "%\n";

	ofstream outfile("compressed.bin", ios::binary);
	if (!outfile.is_open()) {
		cout << "Failed to create compressed.bin\n";
		return 1;
	}

	outfile.write((char*)&tokenCount, sizeof(tokenCount));

	for (int i = 0; i < tokenCount; i += 8) {
		uint8_t flags = 0;
		int groupEnd = min(i + 8, tokenCount);
		for (int j = i; j < groupEnd; ++j) {
			if (compressed[j].isMatch) {
				flags |= (uint8_t)(1u << (j - i));
			}
		}
		outfile.write((char*)&flags, sizeof(flags));

		for (int j = i; j < groupEnd; ++j) {
			if (compressed[j].isMatch) {
				outfile.write((char*)&compressed[j].offset, sizeof(compressed[j].offset));
				outfile.write((char*)&compressed[j].length, sizeof(compressed[j].length));
			}
			else {
				outfile.write((char*)&compressed[j].literal, sizeof(compressed[j].literal));
			}
		}
	}

	outfile.close();

	ofstream txtOutfile("compressed.txt");
	if (!txtOutfile.is_open()) {
		cout << "Failed to create compressed.txt\n";
		return 1;
	}

	for (const auto& token : compressed) {
		if (token.isMatch) {
			txtOutfile << "M " << token.offset << ' ' << token.length << '\n';
		}
		else {
			int nextValue = (unsigned char)token.literal;
			txtOutfile << "L " << nextValue << '\n';
		}
	}
	txtOutfile.close();

	writeLastCaseMetadata(selectedCase);

	cout << "Compression complete. Tokens: " << compressed.size() << "\n";
	cout << "Output file: compressed.bin\n";
	cout << "Text output file: compressed.txt\n";

	return 0;
}