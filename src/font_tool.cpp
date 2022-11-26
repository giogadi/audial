#include <fstream>
#include <vector>
#include <filesystem>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

bool ReadFile(char const* filename, std::vector<char>& data) {
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        return false;
    }
    std::filesystem::path path(filename);
    int numBytes = std::filesystem::file_size(path);
    data.resize(numBytes);
    inFile.read(data.data(), numBytes);
    return true;
}

int main(int argc, char** argv) {
    std::filesystem::path ttfPath(argv[1]);
    std::filesystem::path bmpPath(ttfPath);
    bmpPath.replace_extension("bmp");
    std::filesystem::path infoPath(ttfPath);
    infoPath.replace_extension("info");
    
    std::vector<unsigned char> ttfData;
    {
        std::vector<char> tmp;
        if (!ReadFile(ttfPath.c_str(), tmp)) {
            printf("Failed to open font \"%s\"!\n", ttfPath.c_str());
            return 1;
        }
        ttfData.reserve(tmp.size());
        for (char c : tmp) {
            ttfData.push_back((unsigned char) c);
        }
    }

    int pixelHeight = 48;
    int firstChar = 65;  // A
    int numChars = 58;
    std::vector<unsigned char> bitmap(512 * 128);
    std::vector<stbtt_bakedchar> fontCharInfos(numChars);
    int result = stbtt_BakeFontBitmap(
        ttfData.data(), /*offset=*/0, pixelHeight, bitmap.data(), 512, 128, firstChar, numChars, fontCharInfos.data());
    printf("RESULT: %d\n", result);

    if (stbi_write_bmp(bmpPath.c_str(), 512, 128, /*comp=*/1, bitmap.data()) == 0) {
        printf("Failed to write bitmap \"%s\"!\n", bmpPath.c_str());
        return 1;
    }

    std::ofstream infoFile(infoPath);
    if (!infoFile.is_open()) {
        printf("Failed to write info file to \"%s\"!\n", infoPath.c_str());
        return 1;
    }
    for (stbtt_bakedchar const& c : fontCharInfos) {
        infoFile << c.x0 << " " << c.y0 << " " << c.x1 << " " << c.y1 << " " <<
            c.xoff << " " << c.yoff << " " << c.xadvance << std::endl;
    }
    
    return 0;
}
