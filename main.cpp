#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include "json.hpp"
#include <vector>
#include <cstdio>
#include <filesystem>
#include <cstdint>
#include <variant>

#include "Q1MDL.h"
#include "H2PoPMDL.h"
#include "Q2MD2.h"
#include "DKM.h"
#include "Q3MD3.h"

using namespace std;
using json = nlohmann::ordered_json;
namespace fs = filesystem;

void JSONChoiceSplit(fs::path inpath) {
    ifstream fjson(inpath);
    if (!fjson.is_open()) {
        cout << "Error: Could not open JSON file." << endl;
        return;
    }

    json jsoncontents = json::parse(fjson);
    fjson.close();

    auto jheader = jsoncontents.at("header");
    if (jheader.at("ident").get<string>() == "IDPO" && jheader.at("version").get<int>() == 6) {
        cout << "Identified Quake 1 model JSON." << endl;
        fs::path outPath = inpath;
        outPath.replace_extension(".mdl");
        JSON2Q1MDL(inpath, outPath, jsoncontents);
        return;
    }
    if (jheader.at("ident").get<string>() == "RAPO" && jheader.at("version").get<int>() == 50) {
        cout << "Identified Hexen ][: Portals of Praevus model JSON." << endl;
        fs::path outPath = inpath;
        outPath.replace_extension(".mdl");
        JSON2H2PoPMDL(inpath, outPath, jsoncontents);
        return;
    }
    if (jheader.at("ident").get<string>() == "IDP2" && jheader.at("version").get<int>() == 8) {
        cout << "Identified Quake 2 model JSON." << endl;
        fs::path outPath = inpath;
        outPath.replace_extension(".md2");
        JSON2Q2MD2(inpath, outPath, jsoncontents);
        return;
    }
    if (jheader.at("ident").get<string>() == "DKMD" && jheader.at("version").get<int>() > 0 && jheader.at("version").get<int>() < 3) {
        cout << "Identified Daikatana model file: " << jheader.at("ident").get<string>() << ", version " << jheader.at("version").get<int>() << endl;
        fs::path outPath = inpath;
        outPath.replace_extension(".dkm");
        JSON2DKM(inpath, outPath, jsoncontents);
        return;
    }
    if (jheader.at("ident").get<string>() == "IDP3" && jheader.at("version").get<int>() == 15) {
        cout << "Identified Quake 3 model JSON." << endl;
        fs::path outPath = inpath;
        outPath.replace_extension(".md3");
        JSON2Q3MD3(inpath, outPath, jsoncontents);
        return;
    }

}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "No file provided. Please provide either a supported 3D model, or a valid JSON file." << endl;
        return 1;
    }

    fs::path filePath(argv[1]);

    string extension = filePath.extension().string();
    for (auto& c : extension) c = tolower(c);

    if (extension == ".json") {
        cout << "Processing JSON file..." << endl;
        JSONChoiceSplit(filePath);
    }
    else if (extension == ".mdl") {
        cout << "Processing MDL file..." << endl;
        fs::path outPath = filePath;
        outPath.replace_extension(".json");

        ifstream inFile(filePath, ios::binary);
        if (!inFile.is_open()) {
            cout << "Error: Could not open input file." << endl;
            return 1;
        }

        char ident[4];
        inFile.read(ident, 4);
        int versionnum;
        inFile.read(reinterpret_cast<char*>(&versionnum), sizeof(versionnum));
        inFile.close();

        if (memcmp(ident, "IDPO", 4) == 0 && versionnum == 6) {
            cout << "Identified Quake 1 model file: " << string(ident, 4) << " and " << versionnum << endl;
            Q1MDL2JSON(ParseQ1MDL(filePath), outPath);
        }
        else if (memcmp(ident, "RAPO", 4) == 0 && versionnum == 50) {
            cout << "Identified Hexen ][: Portals of Praevus model file: " << string(ident, 4) << " and " << versionnum << endl;
            H2PoPMDL2JSON(ParseH2PoPMDL(filePath), outPath);
        }
        else {
            cout << "Error: Invalid ident and version numbers: " << string(ident, 4) << " and " << versionnum << endl;
            return 1;
        }
    }
    else if (extension == ".md2") {
        cout << "Processing MD2 file..." << endl;
        fs::path outPath = filePath;
        outPath.replace_extension(".json");

        ifstream inFile(filePath, ios::binary);
        if (!inFile.is_open()) {
            cout << "Error: Could not open input file." << endl;
            return 1;
        }

        char ident[4];
        inFile.read(ident, 4);
        int versionnum;
        inFile.read(reinterpret_cast<char*>(&versionnum), sizeof(versionnum));
        inFile.close();

        if (memcmp(ident, "IDP2", 4) == 0 && versionnum == 8) {
            cout << "Identified Quake 2 model file: " << string(ident, 4) << " and " << versionnum << endl;
            Q2MD22JSON(ParseQ2MD2(filePath), outPath);
        }
        else {
            cout << "Error: Invalid ident and version numbers: " << string(ident, 4) << " and " << versionnum << endl;
            return 1;
        }
    }
    else if (extension == ".dkm") {
        cout << "Processing DKM file..." << endl;
        fs::path outPath = filePath;
        outPath.replace_extension(".json");

        ifstream inFile(filePath, ios::binary);
        if (!inFile.is_open()) {
            cout << "Error: Could not open input file." << endl;
            return 1;
        }

        char ident[4];
        inFile.read(ident, 4);
        uint32_t versionnum;
        inFile.read(reinterpret_cast<char*>(&versionnum), sizeof(versionnum));
        inFile.close();

        if (memcmp(ident, "DKMD", 4) == 0 && versionnum > 0 && versionnum < 3) {
            cout << "Identified Daikatana model file: " << string(ident, 4) << ", version " << versionnum << endl;
            DKM2JSON(ParseDKM(filePath), outPath);
        }
        else {
            cout << "Error: Invalid ident and version numbers: " << string(ident, 4) << " and " << versionnum << endl;
            return 1;
        }
    }
    else if (extension == ".md3") {
        cout << "Processing MD3 file..." << endl;
        fs::path outPath = filePath;
        outPath.replace_extension(".json");

        ifstream inFile(filePath, ios::binary);
        if (!inFile.is_open()) {
            cout << "Error: Could not open input file." << endl;
            return 1;
        }

        char ident[4];
        inFile.read(ident, 4);
        int versionnum;
        inFile.read(reinterpret_cast<char*>(&versionnum), sizeof(versionnum));
        inFile.close();

        if (memcmp(ident, "IDP3", 4) == 0 && versionnum == 15) {
            cout << "Identified Quake 3 model file: " << string(ident, 4) << " and " << versionnum << endl;
            Q3MD32JSON(ParseQ3MD3(filePath), outPath);
        }
        else {
            cout << "Error: Invalid ident and version numbers: " << string(ident, 4) << " and " << versionnum << endl;
            return 1;
        }
    }
    else {
        cout << "Unsupported file extension: " << filePath.extension() << endl;
    }

    return 0;
}
