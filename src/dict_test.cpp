#include <fstream>
#include <string>
#include <cstdio>
#include <vector>
#include <iostream>
#include <cinttypes>
#include <cassert>
#include <algorithm>

struct MyStr {
	int32_t ix;
};

struct Dictionary {
	std::vector<MyStr> wordList;
	std::vector<int> wordTypeCosts;
	std::vector<std::vector<int>> wordsByLengthAndCost;
	char *data;
	int64_t dataSize;

	char const *GetStr(int wordIx, int *len=nullptr) {
		int32_t dataIx = wordList[wordIx].ix;
		char const *s = &data[dataIx];
		if (len) {
			if (wordIx+1 < wordList.size()) {
				*len = (wordList[wordIx+1].ix - dataIx) - 1;
			} else {
				*len = dataSize - dataIx - 1;
			}
		}
		return s;
	}
};

int preprocessDict(char const *filename) {
	int64_t fileSize = 0;
	char *data = nullptr;
	{
		FILE *fp = fopen(filename, "rb");
		fseek(fp, 0, SEEK_END);
		fileSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		int64_t kMaxFileSize = (1 << 20) * 64;  //64 MB
		if (fileSize > kMaxFileSize) {
			printf("NO WAY DUDE\n");
			return 1;
		}
		data = new char[fileSize];
		fread(data, sizeof(char), fileSize, fp);
		fclose(fp);
	}

	std::string outFilename = std::string(filename) + ".out";
	FILE *fp = fopen(outFilename.c_str(), "wb");
	char currentWordLength = 0;
	for (int64_t ii = 0; ii < fileSize; ++ii) {
		char c = data[ii];
		if (c == '\n' || c == '\r') {
			if (currentWordLength > 0) {
				currentWordLength = 0;
				fputc('\0', fp);
			}
		} else {
			++currentWordLength;
			fputc(c, fp);
		}
	}

	fclose(fp);

	delete[] data;
	return 0;
}

enum class Hand { Left, Right };
Hand CharHand(char c) {
	switch (c) {
		case 'a': return Hand::Left;
		case 'b': return Hand::Left;
		case 'c': return Hand::Left;
		case 'd': return Hand::Left;
		case 'e': return Hand::Left;
		case 'f': return Hand::Left;
		case 'g': return Hand::Left;
		case 'h': return Hand::Right;
		case 'i': return Hand::Right;
		case 'j': return Hand::Right;
		case 'k': return Hand::Right;
		case 'l': return Hand::Right;
		case 'm': return Hand::Right;
		case 'n': return Hand::Right;
		case 'o': return Hand::Right;
		case 'p': return Hand::Right;
		case 'q': return Hand::Left;
		case 'r': return Hand::Left;
		case 's': return Hand::Left;
		case 't': return Hand::Left;
		case 'u': return Hand::Right;
		case 'v': return Hand::Left;
		case 'w': return Hand::Left;
		case 'x': return Hand::Left;
		case 'y': return Hand::Right;
		case 'z': return Hand::Left;
		default: assert(false); return Hand::Left;
	}
	return Hand::Left;
}

// 0,1,2,3: index, middle, ring, pinkie (LH)
// 4,5,6,7: same (RH)
int CharFinger(char c) {
	switch (c) {
		case 'a': return 3;
		case 'b': return 0;
		case 'c': return 1;
		case 'd': return 1;
		case 'e': return 1;
		case 'f': return 0;
		case 'g': return 0;
		case 'h': return 4+0;
		case 'i': return 4+1;
		case 'j': return 4+0;
		case 'k': return 4+1;
		case 'l': return 4+2;
		case 'm': return 4+0;
		case 'n': return 4+0;
		case 'o': return 4+2;
		case 'p': return 4+3;
		case 'q': return 3;
		case 'r': return 0;
		case 's': return 2;
		case 't': return 0;
		case 'u': return 4+0;
		case 'v': return 0;
		case 'w': return 2;
		case 'x': return 2;
		case 'y': return 4+0;
		case 'z': return 3;
		default: assert(false); return 0;
	}
	return 0;
}

Hand HandFromFinger(int finger) {
	int x = finger >= 4;
	Hand h = static_cast<Hand>(x);
	return h;
}

int WordTypeCost(char const *str, int len) {
	int prevFinger = 0;
	int cost = 0;
	int constexpr kSameFingerCost = 2;
	int constexpr kSameHandCost = 1;
	//int constexpr kReachCost = 1;
	for (int ii = 0; ii < len; ++ii) {
		char const c = str[ii];
		int finger = CharFinger(c);
		if (ii > 0) {
			if (finger == prevFinger) {
				cost += kSameFingerCost;
			} else if (HandFromFinger(finger) == HandFromFinger(prevFinger)) {
				cost += kSameHandCost;
			}
		}
		prevFinger = finger;
	}
	return cost;
}

int again(char const *filename) {
	Dictionary dict;
	dict.data = nullptr;
	{
		FILE *fp = fopen(filename, "rb");
		fseek(fp, 0, SEEK_END);
		dict.dataSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		int64_t kMaxFileSize = (1 << 20) * 64;  //64 MB
		if (dict.dataSize > kMaxFileSize) {
			printf("NO WAY DUDE\n");
			return 1;
		}
		dict.data = new char[dict.dataSize];
		fread(dict.data, sizeof(char), dict.dataSize, fp);
		fclose(fp);
	}	

	int currentWordStart = 0;
	char currentWordLength = 0;
	for (int64_t ii = 0; ii < dict.dataSize; ++ii) {
		char c = dict.data[ii];
		if (c == '\n' || c == '\r' || c == '\0') {
			dict.data[ii] = '\0';
			if (currentWordLength > 0) {
				MyStr str;
				str.ix = currentWordStart;
				dict.wordList.push_back(str);

				currentWordStart = ii + 1;
				currentWordLength = 0;
			} else {
				++currentWordStart;
			}
		} else {
			++currentWordLength;
		}
	}

	dict.wordTypeCosts.reserve(dict.wordList.size());
	for (int ii = 0; ii < dict.wordList.size(); ++ii) {
		int len;
		char const *str = dict.GetStr(ii, &len);
		int cost = WordTypeCost(str, len);
		dict.wordTypeCosts.push_back(cost);
	}

	int const kMaxWordLength = 10;
	dict.wordsByLengthAndCost.resize(kMaxWordLength+1);
	for (int wordIx = 0; wordIx < dict.wordList.size(); ++wordIx) {
		int len;
		char const *str = dict.GetStr(wordIx, &len);
		if (len <= kMaxWordLength) {
			dict.wordsByLengthAndCost[len].push_back(wordIx);
		}
	}
	for (int wordLength = 0; wordLength < dict.wordsByLengthAndCost.size(); ++wordLength) {
		std::vector<int> &words = dict.wordsByLengthAndCost[wordLength];
		std::sort(words.begin(), words.end(), [&](int lhs, int rhs) {
			return dict.wordTypeCosts[lhs] < dict.wordTypeCosts[rhs];
		});
	}
	
	printf("Word count: %lld\n", dict.wordList.size());
	for (int lengthIx = 0; lengthIx < dict.wordsByLengthAndCost.size(); ++lengthIx) {
		int wc = dict.wordsByLengthAndCost[lengthIx].size();
		printf("%d letters: %d\n", lengthIx, wc);
	}

	int len;
	char const *s = dict.GetStr(dict.wordList.size() - 2, &len);
	printf("len: %d, s: %s\n", len, s);

	for (int ii = 0; ii < 50 && ii < dict.wordsByLengthAndCost[6].size(); ++ii) {
		int wordIx = dict.wordsByLengthAndCost[6][ii];
		int cost = dict.wordTypeCosts[wordIx];
		char const *str = dict.GetStr(wordIx);
		printf("%d: %s\n", cost, str);
	}

	std::string useless;
	std::cin >> useless;
	
	delete [] dict.data;

	return 0;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Need exactly one arg (dict file)\n");
		return 1;
	}

	return again(argv[1]);
	//return preprocessDict(argv[1]);
}