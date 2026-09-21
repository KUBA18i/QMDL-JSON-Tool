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

#include "Q2MD2.h"

using namespace std;
using json = nlohmann::ordered_json;
namespace fs = filesystem;

Q2_MD2_file JSON2Q2MD2(fs::path inpath, json jsonMD2) {
    try {
        Q2_MD2_file newMD2;

        newMD2.header.magic = 844121161;//IDP2
        newMD2.header.version = 8;
        newMD2.header.numSkins = jsonMD2["skins"].size();
        newMD2.header.numTexCoords = jsonMD2["UV"].size();
        newMD2.header.numTriangles = jsonMD2["triangles"].size();
        newMD2.header.numFrames = jsonMD2["frames"].size();

        newMD2.header.numVertices = -1;
        for (const auto& f : jsonMD2["frames"]) {
            if (newMD2.header.numVertices == -1)
                newMD2.header.numVertices = f["verts"].size();
            else if (newMD2.header.numVertices != f["verts"].size()) {
                cout << "Error: Frame " << (f["name"]) << " has " << f["verts"].size() << " verts, but expected " << newMD2.header.numVertices << endl;
                exit(1);
            }
        }

        newMD2.header.numGlCommands = 0;
        for (const auto& g : jsonMD2["glCommands"]) {
            newMD2.header.numGlCommands++;
            newMD2.header.numGlCommands += (g["verts"].size() * 3);
        }
        newMD2.header.numGlCommands++;

        newMD2.header.frameSize = newMD2.header.numVertices * 4 + 40;

        auto jheader = jsonMD2.at("header");
        newMD2.header.skinWidth = jheader["skinWidth"];
        newMD2.header.skinHeight = jheader["skinHeight"];
                        
        newMD2.header.offsetSkins = 68;
        for (const auto& s : jsonMD2["skins"])
            newMD2.skinpaths.push_back(s);
        
        newMD2.header.offsetTexCoords = newMD2.header.offsetSkins + newMD2.header.numSkins * 64;
        for (const auto& uv : jsonMD2["UV"]) {
            q2_md2_textureCoordinate_t newTC;
            newTC.s = uv[0];
            newTC.t = uv[1];
            newMD2.UV.push_back(newTC);
        }
        
        newMD2.header.offsetTriangles = newMD2.header.offsetTexCoords + newMD2.header.numTexCoords * 4;
        for (const auto& tri : jsonMD2["triangles"]) {
            q2_md2_triangle_t newTri;
            newTri.vertexIndices[0] = tri[0];
            newTri.vertexIndices[1] = tri[1];
            newTri.vertexIndices[2] = tri[2];
            newTri.textureIndices[0] = tri[3];
            newTri.textureIndices[1] = tri[4];
            newTri.textureIndices[2] = tri[5];
            newMD2.triangles.push_back(newTri);
        }
        
        newMD2.header.offsetFrames = newMD2.header.offsetTriangles + newMD2.header.numTriangles * 12;
        for (const auto& f : jsonMD2["frames"]) {
            q2_md2_frame_t newFrame;
            newFrame.scale[0] = f["scale"][0];
            newFrame.scale[1] = f["scale"][1];
            newFrame.scale[2] = f["scale"][2];
            newFrame.translate[0] = f["translate"][0];
            newFrame.translate[1] = f["translate"][1];
            newFrame.translate[2] = f["translate"][2];
            string sName = f["name"];
            memset(newFrame.name, 0, sizeof(newFrame.name));
            strncpy(newFrame.name, sName.c_str(), sizeof(newFrame.name) - 1);
            for (const auto& v : f["verts"]) {
                q2_md2_triangleVertex_t newvert;
                newvert.vertex[0] = v[0];
                newvert.vertex[1] = v[1];
                newvert.vertex[2] = v[2];
                newvert.lightNormalIndex = v[3];
                newFrame.vertices.push_back(newvert);
            }
            newMD2.frames.push_back(newFrame);
        }
        
        newMD2.header.offsetGlCommands = newMD2.header.offsetFrames + newMD2.header.frameSize * newMD2.header.numFrames;
        for (const auto& g : jsonMD2["glCommands"]) {
            q2_md2_glCommand_t newGC;
            newGC.count = g["verts"].size();
            if (g["strip"].get<bool>() == false) newGC.count *= (-1);
            for (const auto& v : g["verts"]) {
                q2_md2_glCommandVertex_t newGV;
                newGV.s = v[0];
                newGV.t = v[1];
                newGV.vertexIndex = v[2];
                newGC.vertices.push_back(newGV);
            }
            newMD2.GLCommands.push_back(newGC);
        }
        newMD2.header.offsetEnd = newMD2.header.offsetGlCommands + newMD2.header.numGlCommands * 4;
        cout << "JSON parsed, creating MD2..." << endl;
        return newMD2;
    }
    catch (exception& e) {
        cout << "JSON Parsing Error: " << e.what() << endl;
    }
}

void WriteQ2MD2(fs::path outpath, Q2_MD2_file newMD2) {
    try {
        ofstream outFile(outpath, ios::binary);
        if (!outFile.is_open()) {
            cout << "Error: Could not open output file." << endl;
            return;
        }

        outFile.write("IDP2", 4);
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.version), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.skinWidth), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.skinHeight), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.frameSize), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.numSkins), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.numVertices), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.numTexCoords), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.numTriangles), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.numGlCommands), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.numFrames), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.offsetSkins), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.offsetTexCoords), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.offsetTriangles), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.offsetFrames), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.offsetGlCommands), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&newMD2.header.offsetEnd), sizeof(int));

        for (const auto& s : newMD2.skinpaths) {
            char name[64] = { 0 };
            strncpy(name, s.c_str(), 63);
            outFile.write(name, 64);
        }

        for (const auto& uv : newMD2.UV) {
            outFile.write(reinterpret_cast<const char*>(&uv.s), sizeof(uv.s));
            outFile.write(reinterpret_cast<const char*>(&uv.t), sizeof(uv.t));
        }

        for (const auto& tri : newMD2.triangles)
            outFile.write(reinterpret_cast<const char*>(&tri), sizeof(tri));

        for (const auto& f : newMD2.frames) {
            outFile.write(reinterpret_cast<const char*>(&f.scale), sizeof(f.scale));
            outFile.write(reinterpret_cast<const char*>(&f.translate), sizeof(f.translate));
            outFile.write(f.name, 16);
            for (const auto& v : f.vertices)
                outFile.write(reinterpret_cast<const char*>(&v), sizeof(v));
        }
        
        for (const auto& g : newMD2.GLCommands) {
            outFile.write(reinterpret_cast<const char*>(&g.count), sizeof(g.count));
            for (const auto& v : g.vertices) {
                outFile.write(reinterpret_cast<const char*>(&v.s), sizeof(v.s));
                outFile.write(reinterpret_cast<const char*>(&v.t), sizeof(v.t));
                outFile.write(reinterpret_cast<const char*>(&v.vertexIndex), sizeof(v.vertexIndex));
            }
        }
        outFile.write("\0\0\0\0", 4);
        if (newMD2.header.offsetEnd != outFile.tellp())
            cout << "Warning: offsetEnd doesn't match the end of the file: " << newMD2.header.offsetEnd << " vs " << outFile.tellp() << endl;

        outFile.close();
        cout << "MD2 constructed successfully: " << outpath << endl;
    }
    catch (exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}

Q2_MD2_file ReadQ2MD2(fs::path inpath) {
    ifstream inFile(inpath, ios::binary);
    Q2_MD2_file NewMD2;

    inFile.read(reinterpret_cast<char*>(&NewMD2.header.magic), sizeof(NewMD2.header.magic));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.version), sizeof(NewMD2.header.version));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.skinWidth), sizeof(NewMD2.header.skinWidth));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.skinHeight), sizeof(NewMD2.header.skinHeight));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.frameSize), sizeof(NewMD2.header.frameSize));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.numSkins), sizeof(NewMD2.header.numSkins));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.numVertices), sizeof(NewMD2.header.numVertices));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.numTexCoords), sizeof(NewMD2.header.numTexCoords));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.numTriangles), sizeof(NewMD2.header.numTriangles));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.numGlCommands), sizeof(NewMD2.header.numGlCommands));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.numFrames), sizeof(NewMD2.header.numFrames));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.offsetSkins), sizeof(NewMD2.header.offsetSkins));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.offsetTexCoords), sizeof(NewMD2.header.offsetTexCoords));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.offsetTriangles), sizeof(NewMD2.header.offsetTriangles));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.offsetFrames), sizeof(NewMD2.header.offsetFrames));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.offsetGlCommands), sizeof(NewMD2.header.offsetGlCommands));
    inFile.read(reinterpret_cast<char*>(&NewMD2.header.offsetEnd), sizeof(NewMD2.header.offsetEnd));

    if (NewMD2.header.offsetSkins < NewMD2.header.offsetTexCoords && NewMD2.header.offsetTexCoords < NewMD2.header.offsetTriangles && NewMD2.header.offsetTriangles < NewMD2.header.offsetFrames && NewMD2.header.offsetFrames < NewMD2.header.offsetGlCommands && NewMD2.header.offsetGlCommands < NewMD2.header.offsetEnd) {
        cout << "Offsets are in ascending order." << endl;
    }
    else {
        cout << "WARNING: Offsets are NOT in ascending order." << endl;
        cout << "header.offsetSkins: " << NewMD2.header.offsetSkins << endl;
        cout << "header.offsetTexCoords: " << NewMD2.header.offsetTexCoords << endl;
        cout << "header.offsetTriangles: " << NewMD2.header.offsetTriangles << endl;
        cout << "header.offsetFrames: " << NewMD2.header.offsetFrames << endl;
        cout << "header.offsetGlCommands: " << NewMD2.header.offsetGlCommands << endl;
        cout << "header.offsetEnd: " << NewMD2.header.offsetEnd << endl;
    }

    inFile.seekg(NewMD2.header.offsetSkins);
    for (int i = 0; i < NewMD2.header.numSkins; i++) {
        char newpath[64];
        inFile.read(newpath, 64);
        NewMD2.skinpaths.emplace_back(newpath);
    }

    inFile.seekg(NewMD2.header.offsetTexCoords);
    for (int i = 0; i < NewMD2.header.numTexCoords; i++) {
        q2_md2_textureCoordinate_t newtexcord;
        inFile.read(reinterpret_cast<char*>(&newtexcord.s), sizeof(newtexcord.s));
        inFile.read(reinterpret_cast<char*>(&newtexcord.t), sizeof(newtexcord.t));
        NewMD2.UV.push_back(newtexcord);
    }

    inFile.seekg(NewMD2.header.offsetTriangles);
    for (int i = 0; i < NewMD2.header.numTriangles; i++) {
        q2_md2_triangle_t newtriangle;
        inFile.read(reinterpret_cast<char*>(&newtriangle.vertexIndices[0]), sizeof(newtriangle.vertexIndices[0]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.vertexIndices[1]), sizeof(newtriangle.vertexIndices[1]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.vertexIndices[2]), sizeof(newtriangle.vertexIndices[2]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.textureIndices[0]), sizeof(newtriangle.textureIndices[0]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.textureIndices[1]), sizeof(newtriangle.textureIndices[1]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.textureIndices[2]), sizeof(newtriangle.textureIndices[2]));
        NewMD2.triangles.push_back(newtriangle);
    }

    inFile.seekg(NewMD2.header.offsetFrames);
    for (int i = 0; i < NewMD2.header.numFrames; i++) {
        q2_md2_frame_t newframe;
        inFile.read(reinterpret_cast<char*>(&newframe.scale[0]), sizeof(newframe.scale[0]));
        inFile.read(reinterpret_cast<char*>(&newframe.scale[1]), sizeof(newframe.scale[1]));
        inFile.read(reinterpret_cast<char*>(&newframe.scale[2]), sizeof(newframe.scale[2]));
        inFile.read(reinterpret_cast<char*>(&newframe.translate[0]), sizeof(newframe.translate[0]));
        inFile.read(reinterpret_cast<char*>(&newframe.translate[1]), sizeof(newframe.translate[1]));
        inFile.read(reinterpret_cast<char*>(&newframe.translate[2]), sizeof(newframe.translate[2]));
        inFile.read(newframe.name, 16);
        for (int j = 0; j < NewMD2.header.numVertices; j++) {
            q2_md2_triangleVertex_t newvert;
            inFile.read(reinterpret_cast<char*>(&newvert.vertex[0]), sizeof(newvert.vertex[0]));
            inFile.read(reinterpret_cast<char*>(&newvert.vertex[1]), sizeof(newvert.vertex[1]));
            inFile.read(reinterpret_cast<char*>(&newvert.vertex[2]), sizeof(newvert.vertex[2]));
            inFile.read(reinterpret_cast<char*>(&newvert.lightNormalIndex), sizeof(newvert.lightNormalIndex));
            newframe.vertices.push_back(newvert);
        }
        NewMD2.frames.push_back(newframe);
    }

    inFile.seekg(NewMD2.header.offsetGlCommands);
    while (true) {
        int command;
        inFile.read(reinterpret_cast<char*>(&command), sizeof(int));

        if (command == 0) break; // End of commands

        q2_md2_glCommand_t cmd;
        cmd.count = command;
        int numVerts = abs(command);

        for (int i = 0; i < numVerts; i++) {
            q2_md2_glCommandVertex_t v;
            inFile.read(reinterpret_cast<char*>(&v.s), sizeof(float));
            inFile.read(reinterpret_cast<char*>(&v.t), sizeof(float));
            inFile.read(reinterpret_cast<char*>(&v.vertexIndex), sizeof(int));
            cmd.vertices.push_back(v);
        }
        NewMD2.GLCommands.push_back(cmd);
    }
    inFile.close();

    return NewMD2;
}

void Q2MD22JSON(const Q2_MD2_file& NewMD2, fs::path outpath) {
    json jsonMD2;
    json jheader;
    jheader["ident"] = "IDP2";
    jheader["version"] = NewMD2.header.version;
    jheader["skinWidth"] = NewMD2.header.skinWidth;
    jheader["skinHeight"] = NewMD2.header.skinHeight;
    jheader["frameSize"] = NewMD2.header.frameSize;
    jheader["numSkins"] = NewMD2.header.numSkins;
    jheader["numVertices"] = NewMD2.header.numVertices;
    jheader["numTexCoords"] = NewMD2.header.numTexCoords;
    jheader["numTriangles"] = NewMD2.header.numTriangles;
    jheader["numGlCommands"] = NewMD2.header.numGlCommands;
    jheader["numFrames"] = NewMD2.header.numFrames;
    jheader["offsetSkins"] = NewMD2.header.offsetSkins;
    jheader["offsetTexCoords"] = NewMD2.header.offsetTexCoords;
    jheader["offsetTriangles"] = NewMD2.header.offsetTriangles;
    jheader["offsetFrames"] = NewMD2.header.offsetFrames;
    jheader["offsetGlCommands"] = NewMD2.header.offsetGlCommands;
    jheader["offsetEnd"] = NewMD2.header.offsetEnd;
    jsonMD2["header"] = jheader;

    jsonMD2["skins"] = json::array();;
    for (const auto& skinpath : NewMD2.skinpaths) {
        json s;
        s = skinpath;
        jsonMD2["skins"].push_back(s);
    }

    jsonMD2["UV"] = json::array();;
    for (const auto& uve : NewMD2.UV) {
        json s;
        s = { uve.s, uve.t };
        jsonMD2["UV"].push_back(s);
    }

    jsonMD2["triangles"] = json::array();;
    for (const auto& tri : NewMD2.triangles) {
        json s;
        s = { tri.vertexIndices[0], tri.vertexIndices[1], tri.vertexIndices[2], tri.textureIndices[0], tri.textureIndices[1] , tri.textureIndices[2] };
        jsonMD2["triangles"].push_back(s);
    }

    jsonMD2["frames"] = json::array();
    for (const auto& frame : NewMD2.frames) {
        json jframe;
        jframe["scale"] = { frame.scale[0],frame.scale[1],frame.scale[2] };
        jframe["translate"] = { frame.translate[0], frame.translate[1], frame.translate[2] };
        jframe["name"] = frame.name;
        jframe["verts"] = json::array();
        for (int i = 0; i < NewMD2.header.numVertices; i++) {
            json jvert;
            jvert = { frame.vertices[i].vertex[0],frame.vertices[i].vertex[1], frame.vertices[i].vertex[2], frame.vertices[i].lightNormalIndex };
            jframe["verts"].push_back(jvert);
        }
        jsonMD2["frames"].push_back(jframe);

    }

    jsonMD2["glCommands"] = json::array();
    for (const auto& cmd : NewMD2.GLCommands) {
        json jCmd;
        jCmd["strip"] = (cmd.count > 0);
        jCmd["verts"] = json::array();
        for (const auto& v : cmd.vertices)
            jCmd["verts"].push_back({ v.s, v.t, v.vertexIndex });
        jsonMD2["glCommands"].push_back(jCmd);
    }

    cout << "JSON prepped, time to export." << endl;
    ofstream outFile(outpath);
    if (outFile.is_open()) {
        outFile << jsonMD2.dump(2);
        outFile.close();
        cout << "JSON created successfully: " << outpath << endl;
    }
    else {
        cout << "Error: Could not write output file." << endl;
    }
}