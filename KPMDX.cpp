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

#include "KPMDX.h"

using namespace std;
using json = nlohmann::ordered_json;
namespace fs = filesystem;

void JSON2KPMDX(fs::path inpath, fs::path outpath, json jsonMDX) {
    try {
        ofstream outFile(outpath, ios::binary);
        if (!outFile.is_open()) {
            cout << "Error: Could not open output file." << endl;
            return;
        }

        kp_mdx_header_t new_header;

        new_header.version = 8;
        new_header.numSkins = jsonMDX["skins"].size();
        new_header.numTexCoords = jsonMDX["UV"].size();
        new_header.numTriangles = jsonMDX["triangles"].size();
        new_header.numFrames = jsonMDX["frames"].size();

        new_header.numVertices = -1;
        for (const auto& f : jsonMDX["frames"]) {
            if (new_header.numVertices == -1)
                new_header.numVertices = f["verts"].size();
            else if (new_header.numVertices != f["verts"].size()) {
                cout << "Error: Frame " << (f["name"]) << " has " << f["verts"].size() << " verts, but expected " << new_header.numVertices << endl;
                return;
            }
        }

        new_header.numGlCommands = 0;
        for (const auto& g : jsonMDX["glCommands"]) {
            new_header.numGlCommands++;
            new_header.numGlCommands += (g["verts"].size() * 3);
        }
        new_header.numGlCommands++;

        new_header.frameSize = new_header.numVertices * 4 + 40;

        auto jheader = jsonMDX.at("header");
        new_header.skinWidth = jheader["skinWidth"];
        new_header.skinHeight = jheader["skinHeight"];
        
        outFile.write("IDP2", 4);
        outFile.write(reinterpret_cast<char*>(&new_header.version), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.skinWidth), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.skinHeight), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.frameSize), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numSkins), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numVertices), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numTexCoords), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numTriangles), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numGlCommands), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numFrames), sizeof(int));
        for (int i = 0; i < 24; i++) outFile.write("X", 1);
        
        new_header.offsetSkins = outFile.tellp();
        for (const auto& s : jsonMDX["skins"]) {
            char name[64] = { 0 };
            string sName = s;
            strncpy(name, sName.c_str(), 63);
            outFile.write(name, 64);
        }
        
        new_header.offsetTexCoords = outFile.tellp();
        for (const auto& uv : jsonMDX["UV"]) {
            short s = uv[0];
            short t = uv[1];
            outFile.write(reinterpret_cast<char*>(&s), sizeof(s));
            outFile.write(reinterpret_cast<char*>(&t), sizeof(t));
        }
        
        new_header.offsetTriangles = outFile.tellp();
        for (const auto& tri : jsonMDX["triangles"]) {
            short t[6] = { tri[0], tri[1], tri[2], tri[3], tri[4], tri[5] };
            outFile.write(reinterpret_cast<char*>(t), sizeof(t));
        }
        
        new_header.offsetFrames = outFile.tellp();
        for (const auto& f : jsonMDX["frames"]) {
            float scale[3] = { f["scale"][0], f["scale"][1], f["scale"][2]};
            float translate[3] = { f["translate"][0], f["translate"][1], f["translate"][2] };
            char name[16] = { 0 };
            string sName = f["name"];
            strncpy(name, sName.c_str(), 15);

            outFile.write(reinterpret_cast<char*>(scale), sizeof(scale));
            outFile.write(reinterpret_cast<char*>(translate), sizeof(translate));
            outFile.write(name, 16);

            for (const auto& v : f["verts"]) {
                byte vert[4] = { v[0], v[1], v[2], v[3] };
                outFile.write(reinterpret_cast<char*>(&vert), 4);
            }
        }
        
        new_header.offsetGlCommands = outFile.tellp();
        for (const auto& g : jsonMDX["glCommands"]) {
            int vcount = g["verts"].size();
            if (g["strip"].get<bool>() == false) vcount *= (-1);
            outFile.write(reinterpret_cast<char*>(&vcount), sizeof(vcount));
            for (const auto& v : g["verts"]) {
                float vs = v[0];
                float vt = v[1];
                int vertexIndex = v[2];
                outFile.write(reinterpret_cast<char*>(&vs), sizeof(vs));
                outFile.write(reinterpret_cast<char*>(&vt), sizeof(vt));
                outFile.write(reinterpret_cast<char*>(&vertexIndex), sizeof(vertexIndex));
            }
        }
        outFile.write("\0\0\0\0", 4);
        new_header.offsetEnd = outFile.tellp();

        outFile.seekp(44);
        outFile.write(reinterpret_cast<char*>(&new_header.offsetSkins), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetTexCoords), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetTriangles), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetFrames), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetGlCommands), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetEnd), sizeof(int));

        outFile.close();
        cout << "MDX constructed successfully: " << outpath << endl;

    }
    catch (exception& e) {
        cout << "JSON Parsing Error: " << e.what() << endl;
    }
}

kp_mdx_file ParseKPMDX(fs::path inpath) {
    ifstream inFile(inpath, ios::binary);
    kp_mdx_file NewMDX;

    inFile.read(reinterpret_cast<char*>(&NewMDX.header.magic), sizeof(NewMDX.header.magic));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.version), sizeof(NewMDX.header.version));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.skinWidth), sizeof(NewMDX.header.skinWidth));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.skinHeight), sizeof(NewMDX.header.skinHeight));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.frameSize), sizeof(NewMDX.header.frameSize));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.numSkins), sizeof(NewMDX.header.numSkins));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.numVertices), sizeof(NewMDX.header.numVertices));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.numTexCoords), sizeof(NewMDX.header.numTexCoords));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.numTriangles), sizeof(NewMDX.header.numTriangles));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.numGlCommands), sizeof(NewMDX.header.numGlCommands));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.numFrames), sizeof(NewMDX.header.numFrames));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetSkins), sizeof(NewMDX.header.offsetSkins));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetTexCoords), sizeof(NewMDX.header.offsetTexCoords));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetTriangles), sizeof(NewMDX.header.offsetTriangles));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetFrames), sizeof(NewMDX.header.offsetFrames));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetGlCommands), sizeof(NewMDX.header.offsetGlCommands));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetEnd), sizeof(NewMDX.header.offsetEnd));

    if (NewMDX.header.offsetSkins < NewMDX.header.offsetTexCoords && NewMDX.header.offsetTexCoords < NewMDX.header.offsetTriangles && NewMDX.header.offsetTriangles < NewMDX.header.offsetFrames && NewMDX.header.offsetFrames < NewMDX.header.offsetGlCommands && NewMDX.header.offsetGlCommands < NewMDX.header.offsetEnd) {
        cout << "Offsets are in ascending order." << endl;
    }
    else {
        cout << "WARNING: Offsets are NOT in ascending order." << endl;
        cout << "header.offsetSkins: " << NewMDX.header.offsetSkins << endl;
        cout << "header.offsetTexCoords: " << NewMDX.header.offsetTexCoords << endl;
        cout << "header.offsetTriangles: " << NewMDX.header.offsetTriangles << endl;
        cout << "header.offsetFrames: " << NewMDX.header.offsetFrames << endl;
        cout << "header.offsetGlCommands: " << NewMDX.header.offsetGlCommands << endl;
        cout << "header.offsetEnd: " << NewMDX.header.offsetEnd << endl;
    }

    inFile.seekg(NewMDX.header.offsetSkins);
    for (int i = 0; i < NewMDX.header.numSkins; i++) {
        char newpath[64];
        inFile.read(newpath, 64);
        NewMDX.skinpaths.emplace_back(newpath);
    }

    inFile.seekg(NewMDX.header.offsetTexCoords);
    for (int i = 0; i < NewMDX.header.numTexCoords; i++) {
        kp_mdx_textureCoordinate_t newtexcord;
        inFile.read(reinterpret_cast<char*>(&newtexcord.s), sizeof(newtexcord.s));
        inFile.read(reinterpret_cast<char*>(&newtexcord.t), sizeof(newtexcord.t));
        NewMDX.UV.push_back(newtexcord);
    }

    inFile.seekg(NewMDX.header.offsetTriangles);
    for (int i = 0; i < NewMDX.header.numTriangles; i++) {
        kp_mdx_triangle_t newtriangle;
        inFile.read(reinterpret_cast<char*>(&newtriangle.vertexIndices[0]), sizeof(newtriangle.vertexIndices[0]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.vertexIndices[1]), sizeof(newtriangle.vertexIndices[1]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.vertexIndices[2]), sizeof(newtriangle.vertexIndices[2]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.textureIndices[0]), sizeof(newtriangle.textureIndices[0]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.textureIndices[1]), sizeof(newtriangle.textureIndices[1]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.textureIndices[2]), sizeof(newtriangle.textureIndices[2]));
        NewMDX.triangles.push_back(newtriangle);
    }

    inFile.seekg(NewMDX.header.offsetFrames);
    for (int i = 0; i < NewMDX.header.numFrames; i++) {
        kp_mdx_frame_t newframe;
        inFile.read(reinterpret_cast<char*>(&newframe.scale[0]), sizeof(newframe.scale[0]));
        inFile.read(reinterpret_cast<char*>(&newframe.scale[1]), sizeof(newframe.scale[1]));
        inFile.read(reinterpret_cast<char*>(&newframe.scale[2]), sizeof(newframe.scale[2]));
        inFile.read(reinterpret_cast<char*>(&newframe.translate[0]), sizeof(newframe.translate[0]));
        inFile.read(reinterpret_cast<char*>(&newframe.translate[1]), sizeof(newframe.translate[1]));
        inFile.read(reinterpret_cast<char*>(&newframe.translate[2]), sizeof(newframe.translate[2]));
        inFile.read(newframe.name, 16);
        for (int j = 0; j < NewMDX.header.numVertices; j++) {
            kp_mdx_triangleVertex_t newvert;
            inFile.read(reinterpret_cast<char*>(&newvert.vertex[0]), sizeof(newvert.vertex[0]));
            inFile.read(reinterpret_cast<char*>(&newvert.vertex[1]), sizeof(newvert.vertex[1]));
            inFile.read(reinterpret_cast<char*>(&newvert.vertex[2]), sizeof(newvert.vertex[2]));
            inFile.read(reinterpret_cast<char*>(&newvert.lightNormalIndex), sizeof(newvert.lightNormalIndex));
            newframe.vertices.push_back(newvert);
        }
        NewMDX.frames.push_back(newframe);
    }

    inFile.seekg(NewMDX.header.offsetGlCommands);
    while (true) {
        int command;
        inFile.read(reinterpret_cast<char*>(&command), sizeof(int));

        if (command == 0) break; // End of commands

        kp_mdx_glCommand_t cmd;
        cmd.count = command;
        int numVerts = abs(command);

        for (int i = 0; i < numVerts; i++) {
            kp_mdx_glCommandVertex_t v;
            inFile.read(reinterpret_cast<char*>(&v.s), sizeof(float));
            inFile.read(reinterpret_cast<char*>(&v.t), sizeof(float));
            inFile.read(reinterpret_cast<char*>(&v.vertexIndex), sizeof(int));
            cmd.vertices.push_back(v);
        }
        NewMDX.GLCommands.push_back(cmd);
    }
    inFile.close();

    return NewMDX;
}

void KPMDX2JSON(const kp_mdx_file& NewMDX, fs::path outpath) {
    json jsonMDX;
    json jheader;
    jheader["ident"] = "IDP2";
    jheader["version"] = NewMDX.header.version;
    jheader["skinWidth"] = NewMDX.header.skinWidth;
    jheader["skinHeight"] = NewMDX.header.skinHeight;
    jheader["frameSize"] = NewMDX.header.frameSize;
    jheader["numSkins"] = NewMDX.header.numSkins;
    jheader["numVertices"] = NewMDX.header.numVertices;
    jheader["numTexCoords"] = NewMDX.header.numTexCoords;
    jheader["numTriangles"] = NewMDX.header.numTriangles;
    jheader["numGlCommands"] = NewMDX.header.numGlCommands;
    jheader["numFrames"] = NewMDX.header.numFrames;
    jheader["offsetSkins"] = NewMDX.header.offsetSkins;
    jheader["offsetTexCoords"] = NewMDX.header.offsetTexCoords;
    jheader["offsetTriangles"] = NewMDX.header.offsetTriangles;
    jheader["offsetFrames"] = NewMDX.header.offsetFrames;
    jheader["offsetGlCommands"] = NewMDX.header.offsetGlCommands;
    jheader["offsetEnd"] = NewMDX.header.offsetEnd;
    jsonMDX["header"] = jheader;

    jsonMDX["skins"] = json::array();;
    for (const auto& skinpath : NewMDX.skinpaths) {
        json s;
        s = skinpath;
        jsonMDX["skins"].push_back(s);
    }

    jsonMDX["UV"] = json::array();;
    for (const auto& uve : NewMDX.UV) {
        json s;
        s = { uve.s, uve.t };
        jsonMDX["UV"].push_back(s);
    }

    jsonMDX["triangles"] = json::array();;
    for (const auto& tri : NewMDX.triangles) {
        json s;
        s = { tri.vertexIndices[0], tri.vertexIndices[1], tri.vertexIndices[2], tri.textureIndices[0], tri.textureIndices[1] , tri.textureIndices[2] };
        jsonMDX["triangles"].push_back(s);
    }

    jsonMDX["frames"] = json::array();
    for (const auto& frame : NewMDX.frames) {
        json jframe;
        jframe["scale"] = { frame.scale[0],frame.scale[1],frame.scale[2] };
        jframe["translate"] = { frame.translate[0], frame.translate[1], frame.translate[2] };
        jframe["name"] = frame.name;
        jframe["verts"] = json::array();
        for (int i = 0; i < NewMDX.header.numVertices; i++) {
            json jvert;
            jvert = { frame.vertices[i].vertex[0],frame.vertices[i].vertex[1], frame.vertices[i].vertex[2], frame.vertices[i].lightNormalIndex };
            jframe["verts"].push_back(jvert);
        }
        jsonMDX["frames"].push_back(jframe);

    }

    jsonMDX["glCommands"] = json::array();
    for (const auto& cmd : NewMDX.GLCommands) {
        json jCmd;
        jCmd["strip"] = (cmd.count > 0);
        jCmd["verts"] = json::array();
        for (const auto& v : cmd.vertices)
            jCmd["verts"].push_back({ v.s, v.t, v.vertexIndex });
        jsonMDX["glCommands"].push_back(jCmd);
    }

    cout << "JSON prepped, time to export." << endl;
    ofstream outFile(outpath);
    if (outFile.is_open()) {
        outFile << jsonMDX.dump(2);
        outFile.close();
        cout << "JSON created successfully: " << outpath << endl;
    }
    else {
        cout << "Error: Could not write output file." << endl;
    }
}