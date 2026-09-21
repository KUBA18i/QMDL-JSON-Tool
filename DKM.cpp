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

#include "DKM.h"

using namespace std;
using json = nlohmann::ordered_json;
namespace fs = filesystem;

DKM_file JSON2DKM(fs::path inpath, json jsonDKM) {
    try {
        
        DKM_file newDKM;
        newDKM.header.ident = 1145916228;//DKMD
        auto jheader = jsonDKM.at("header");
        newDKM.header.version = jheader["version"];
        newDKM.header.origin[0] = jheader["origin"][0];
        newDKM.header.origin[1] = jheader["origin"][1];
        newDKM.header.origin[2] = jheader["origin"][2];
        
        newDKM.header.numSkins = jsonDKM["skins"].size();
        newDKM.header.numTexCoords = jsonDKM["UV"].size();
        newDKM.header.numTriangles = jsonDKM["triangles"].size();
        newDKM.header.numFrames = jsonDKM["frames"].size();
        newDKM.header.numSurfaces = jsonDKM["surfaces"].size();
        newDKM.header.numSequences = jsonDKM["animSeqs"].size();

        newDKM.header.numVertices = -1;
        for (const auto& f : jsonDKM["frames"]) {
            if (newDKM.header.numVertices == -1)
                newDKM.header.numVertices = f["verts"].size();
            else if (newDKM.header.numVertices != f["verts"].size()) {
                cout << "Error: Frame " << (f["name"]) << " has " << f["verts"].size() << " verts, but expected " << newDKM.header.numVertices << endl;
                exit(1);
            }
        }

        newDKM.header.numGlCommands = 0;
        for (const auto& g : jsonDKM["glCommands"]) {
            newDKM.header.numGlCommands += 3;
            newDKM.header.numGlCommands += (g["verts"].size() * 3);
        }
        newDKM.header.numGlCommands++;

        if (newDKM.header.version == 1) {
            newDKM.header.frameSize = newDKM.header.numVertices * 4 + 40;
        }
        else if (newDKM.header.version == 2) {
            newDKM.header.frameSize = newDKM.header.numVertices * 5 + 43;
        }
        
        if (newDKM.header.numSequences > 0)
            newDKM.header.offsetSkins = 88;
        else
            newDKM.header.offsetSkins = 80;
        
        for (const auto& s : jsonDKM["skins"])
            newDKM.skinpaths.push_back(s);

        newDKM.header.offsetTexCoords = newDKM.header.offsetSkins + newDKM.header.numSkins * 64;
        for (const auto& uv : jsonDKM["UV"]) {
            dkm_textureCoordinate_t newTC;
            newTC.s = uv[0];
            newTC.t = uv[1];
            newDKM.UV.push_back(newTC);
        }
        
        newDKM.header.offsetTriangles = newDKM.header.offsetTexCoords + newDKM.header.numTexCoords * 4;
        for (const auto& tri : jsonDKM["triangles"]) {
            dkm_triangle_t newTri;
            newTri.surfaceIndex = tri["surfaceIndex"];
            newTri.num_uvframes = tri["num_uvframes"];
            newTri.vertexIndices[0] = tri["vertexIndices"][0];
            newTri.vertexIndices[1] = tri["vertexIndices"][1];
            newTri.vertexIndices[2] = tri["vertexIndices"][2];
            newTri.textureIndices[0] = tri["textureIndices"][0];
            newTri.textureIndices[1] = tri["textureIndices"][1];
            newTri.textureIndices[2] = tri["textureIndices"][2];
            newDKM.triangles.push_back(newTri);
        }
        
        newDKM.header.offsetFrames = newDKM.header.offsetTriangles + newDKM.header.numTriangles * 16;
        for (const auto& f : jsonDKM["frames"]) {
            dkm_frame_t newFrame;
            newFrame.scale[0] = f["scale"][0];
            newFrame.scale[1] = f["scale"][1];
            newFrame.scale[2] = f["scale"][2];
            newFrame.translate[0] = f["translate"][0];
            newFrame.translate[1] = f["translate"][1];
            newFrame.translate[2] = f["translate"][2];
            newFrame.name = f["name"];
            for (const auto& v : f["verts"]) {
                dkm_triangleVertex_t newVert;
                newVert.vertex[0] = v[0];
                newVert.vertex[1] = v[1];
                newVert.vertex[2] = v[2];
                newVert.lightNormalIndex = v[3];
                newFrame.vertices.push_back(newVert);
            }
            if (newDKM.header.version == 2) {
                newFrame.unknown[0] = f["unknown"][0];
                newFrame.unknown[1] = f["unknown"][1];
                newFrame.unknown[2] = f["unknown"][2];
            }
            newDKM.frames.push_back(newFrame);
        }
        
        newDKM.header.offsetGlCommands = newDKM.header.offsetFrames + newDKM.header.numFrames * newDKM.header.frameSize;
        for (const auto& g : jsonDKM["glCommands"]) {
            dkm_glCommand_t newGLC;
            newGLC.count = g["verts"].size();
            if (g["strip"].get<bool>() == false) newGLC.count *= (-1);
            newGLC.skinIndex = g["skinIndex"];
            newGLC.surfIndex = g["surfIndex"];
            for (const auto& v : g["verts"]) {
                dkm_glCommandVertex_t newGLV;
                newGLV.vertexIndex = v[0];
                newGLV.s = v[1];
                newGLV.t = v[2];
                newGLC.vertices.push_back(newGLV);
            }
            newDKM.GLCommands.push_back(newGLC);
        }

        newDKM.header.offsetSurfaces = newDKM.header.offsetGlCommands + newDKM.header.numGlCommands * 4;
        for (const auto& s : jsonDKM["surfaces"]) {
            dkm_surface_t newSurf;
            newSurf.name = s["name"];
            newSurf.flags = s["flags"];
            newSurf.skinIndex = s["skinIndex"];
            newSurf.skinWidth = s["skinWidth"];
            newSurf.skinHeight = s["skinHeight"];
            newSurf.numUVframes = s["numUVframes"];
            newDKM.surfaces.push_back(newSurf);
        }
        
        if (newDKM.header.numSequences > 0) {
            newDKM.header.offsetSequences = newDKM.header.offsetSurfaces + newDKM.header.numSurfaces * 52;
            for (const auto& as : jsonDKM["animSeqs"]) {
                dkm_animSeq_t newAS;
                newAS.name = as["name"];
                newAS.startFrame = as["startFrame"];
                newAS.endFrame = as["endFrame"];
                newDKM.sequences.push_back(newAS);
            }
            newDKM.header.offsetEnd = newDKM.header.offsetSequences + newDKM.header.numSequences * 24;
        }
        else
            newDKM.header.offsetEnd = newDKM.header.offsetSurfaces + newDKM.header.numSurfaces * 52;
        
        cout << "JSON parsed, creating DKM..." << endl;
        return newDKM;
    }
    catch (exception& e) {
        cout << "JSON Parsing Error: " << e.what() << endl;
    }
}

void WriteDKM(fs::path outpath, DKM_file newDKM) {
    try {
        ofstream outFile(outpath, ios::binary);
        if (!outFile.is_open()) {
            cout << "Error: Could not open output file." << endl;
            return;
        }

        outFile.write("DKMD", 4);
        outFile.write(reinterpret_cast<char*>(&newDKM.header.version), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.origin[0]), sizeof(newDKM.header.origin[0]));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.origin[1]), sizeof(newDKM.header.origin[1]));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.origin[2]), sizeof(newDKM.header.origin[2]));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.frameSize), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.numSkins), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.numVertices), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.numTexCoords), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.numTriangles), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.numGlCommands), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.numFrames), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.numSurfaces), sizeof(int));

        outFile.write(reinterpret_cast<char*>(&newDKM.header.offsetSkins), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.offsetTexCoords), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.offsetTriangles), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.offsetFrames), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.offsetGlCommands), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.offsetSurfaces), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newDKM.header.offsetEnd), sizeof(int));

        if (newDKM.header.numSequences > 0) {
            outFile.write(reinterpret_cast<char*>(&newDKM.header.numSequences), sizeof(int));
            outFile.write(reinterpret_cast<char*>(&newDKM.header.offsetSequences), sizeof(int));
            cout << "Animation sequence data block is present." << endl;
        }
        else
            cout << "Animation sequence data block is absent." << endl;

        for (const auto& s : newDKM.skinpaths) {
            char name[64] = { 0 };
            strncpy(name, s.c_str(), 63);
            outFile.write(name, 64);
        }

        for (auto& uv : newDKM.UV) {
            outFile.write(reinterpret_cast<char*>(&uv.s), sizeof(uv.s));
            outFile.write(reinterpret_cast<char*>(&uv.t), sizeof(uv.t));
        }

        for (auto& tri : newDKM.triangles) {
            outFile.write(reinterpret_cast<char*>(&tri.surfaceIndex), sizeof(tri.surfaceIndex));
            outFile.write(reinterpret_cast<char*>(&tri.num_uvframes), sizeof(tri.num_uvframes));
            outFile.write(reinterpret_cast<char*>(&tri.vertexIndices), sizeof(tri.vertexIndices));
            outFile.write(reinterpret_cast<char*>(&tri.textureIndices), sizeof(tri.textureIndices));
        }

        for (auto& f : newDKM.frames) {
            char name[16] = { 0 };
            strncpy(name, f.name.c_str(), 15);
            outFile.write(reinterpret_cast<char*>(&f.scale), sizeof(f.scale));
            outFile.write(reinterpret_cast<char*>(&f.translate), sizeof(f.translate));
            outFile.write(name, 16);

            for (const auto& v : f.vertices) {
                if (newDKM.header.version == 1) {
                    uint8_t x = static_cast<uint8_t>(v.vertex[0]);
                    uint8_t y = static_cast<uint8_t>(v.vertex[1]);
                    uint8_t z = static_cast<uint8_t>(v.vertex[2]);
                    outFile.write(reinterpret_cast<char*>(&x), sizeof(x));
                    outFile.write(reinterpret_cast<char*>(&y), sizeof(y));
                    outFile.write(reinterpret_cast<char*>(&z), sizeof(z));
                }
                else if (newDKM.header.version == 2) {
                    uint32_t x = static_cast<uint32_t>(v.vertex[0]);
                    uint32_t y = static_cast<uint32_t>(v.vertex[1]);
                    uint32_t z = static_cast<uint32_t>(v.vertex[2]);
                    uint32_t packvert = ((x & 0x7FF) << 21) | ((y & 0x3FF) << 11) | ((z & 0x7FF));
                    outFile.write(reinterpret_cast<char*>(&packvert), sizeof(packvert));
                }
                uint8_t lightNormalIndex = v.lightNormalIndex;
                outFile.write(reinterpret_cast<char*>(&lightNormalIndex), sizeof(lightNormalIndex));
            }
            if (newDKM.header.version == 2)
                outFile.write(reinterpret_cast<char*>(&f.unknown), sizeof(f.unknown));
        }

        for (auto& g : newDKM.GLCommands) {
            outFile.write(reinterpret_cast<char*>(&g.count), sizeof(g.count));
            outFile.write(reinterpret_cast<char*>(&g.skinIndex), sizeof(g.skinIndex));
            outFile.write(reinterpret_cast<char*>(&g.surfIndex), sizeof(g.surfIndex));
            for (auto& v : g.vertices) {
                outFile.write(reinterpret_cast<char*>(&v.vertexIndex), sizeof(v.vertexIndex));
                outFile.write(reinterpret_cast<char*>(&v.s), sizeof(v.s));
                outFile.write(reinterpret_cast<char*>(&v.t), sizeof(v.t));
            }
        }
        outFile.write("\0\0\0\0", 4);

        for (auto& s : newDKM.surfaces) {
            char name[32] = { 0 };
            strncpy(name, s.name.c_str(), 31);
            outFile.write(name, 32);
            outFile.write(reinterpret_cast<char*>(&s.flags), sizeof(s.flags));
            outFile.write(reinterpret_cast<char*>(&s.skinIndex), sizeof(s.skinIndex));
            outFile.write(reinterpret_cast<char*>(&s.skinWidth), sizeof(s.skinWidth));
            outFile.write(reinterpret_cast<char*>(&s.skinHeight), sizeof(s.skinHeight));
            outFile.write(reinterpret_cast<char*>(&s.numUVframes), sizeof(s.numUVframes));
        }

        if (newDKM.header.numSequences > 0) {
            for (auto& as : newDKM.sequences) {
                char name[16] = { 0 };
                strncpy(name, as.name.c_str(), 15);
                outFile.write(name, 16);
                outFile.write(reinterpret_cast<char*>(&as.startFrame), sizeof(as.startFrame));
                outFile.write(reinterpret_cast<char*>(&as.endFrame), sizeof(as.endFrame));
            }
        }

        if (newDKM.header.offsetEnd != outFile.tellp())
            cout << "Warning: offsetEnd doesn't match the end of the file: " << newDKM.header.offsetEnd << " vs " << outFile.tellp() << endl;

        outFile.close();
        cout << "DKM constructed successfully: " << outpath << endl;

    }
    catch (exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}

DKM_file ReadDKM(fs::path inpath) {
    ifstream inFile(inpath, ios::binary);
    DKM_file NewDKM;

    inFile.read(reinterpret_cast<char*>(&NewDKM.header.ident), sizeof(NewDKM.header.ident));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.version), sizeof(NewDKM.header.version));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.origin[0]), sizeof(NewDKM.header.origin[0]));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.origin[1]), sizeof(NewDKM.header.origin[1]));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.origin[2]), sizeof(NewDKM.header.origin[2]));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.frameSize), sizeof(NewDKM.header.frameSize));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.numSkins), sizeof(NewDKM.header.numSkins));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.numVertices), sizeof(NewDKM.header.numVertices));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.numTexCoords), sizeof(NewDKM.header.numTexCoords));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.numTriangles), sizeof(NewDKM.header.numTriangles));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.numGlCommands), sizeof(NewDKM.header.numGlCommands));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.numFrames), sizeof(NewDKM.header.numFrames));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.numSurfaces), sizeof(NewDKM.header.numSurfaces));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.offsetSkins), sizeof(NewDKM.header.offsetSkins));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.offsetTexCoords), sizeof(NewDKM.header.offsetTexCoords));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.offsetTriangles), sizeof(NewDKM.header.offsetTriangles));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.offsetFrames), sizeof(NewDKM.header.offsetFrames));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.offsetGlCommands), sizeof(NewDKM.header.offsetGlCommands));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.offsetSurfaces), sizeof(NewDKM.header.offsetSurfaces));
    inFile.read(reinterpret_cast<char*>(&NewDKM.header.offsetEnd), sizeof(NewDKM.header.offsetEnd));
    if (NewDKM.header.offsetSkins == 80) {
        cout << "Animation sequence data block is absent." << endl;
        NewDKM.header.numSequences = 0;
        NewDKM.header.offsetSequences = NewDKM.header.offsetEnd;
    }
    else if (NewDKM.header.offsetSkins == 88) {
        cout << "Animation sequence data block is present." << endl;
        inFile.read(reinterpret_cast<char*>(&NewDKM.header.numSequences), sizeof(NewDKM.header.numSequences));
        inFile.read(reinterpret_cast<char*>(&NewDKM.header.offsetSequences), sizeof(NewDKM.header.offsetSequences));
    }
    else {
        cout << "Error: Strange number encountered for skin offset: " << NewDKM.header.offsetSkins << endl;
        cout << "The DKM file is likely corrupted." << endl;
        exit(1);
    }

    if (NewDKM.header.offsetSkins < NewDKM.header.offsetTexCoords && NewDKM.header.offsetTexCoords < NewDKM.header.offsetTriangles
        && NewDKM.header.offsetTriangles < NewDKM.header.offsetFrames && NewDKM.header.offsetFrames < NewDKM.header.offsetGlCommands
        && NewDKM.header.offsetGlCommands < NewDKM.header.offsetSurfaces && NewDKM.header.offsetSurfaces < NewDKM.header.offsetSequences && NewDKM.header.offsetSequences <= NewDKM.header.offsetEnd) {
        cout << "Offsets are in ascending order." << endl;
    }
    else {
        cout << "WARNING: Offsets are NOT in ascending order." << endl;
        cout << "header.offsetSkins: " << NewDKM.header.offsetSkins << endl;
        cout << "header.offsetTexCoords: " << NewDKM.header.offsetTexCoords << endl;
        cout << "header.offsetTriangles: " << NewDKM.header.offsetTriangles << endl;
        cout << "header.offsetFrames: " << NewDKM.header.offsetFrames << endl;
        cout << "header.offsetGlCommands: " << NewDKM.header.offsetGlCommands << endl;
        cout << "header.offsetSurfaces: " << NewDKM.header.offsetSurfaces << endl;
        cout << "header.offsetSequences: " << NewDKM.header.offsetSequences << endl;
        cout << "header.offsetEnd: " << NewDKM.header.offsetEnd << endl;
    }
    /*debutprint
    cout << "header.ident: " << NewDKM.header.ident << endl;
    cout << "header.version: " << NewDKM.header.version << endl;
    cout << "header.origin[0]: " << NewDKM.header.origin[0] << endl;
    cout << "header.origin[1]: " << NewDKM.header.origin[1] << endl;
    cout << "header.origin[2]: " << NewDKM.header.origin[2] << endl;
    cout << "header.framesize: " << NewDKM.header.frameSize << endl;
    cout << "header.numSkins: " << NewDKM.header.numSkins << endl;
    cout << "header.numVertices: " << NewDKM.header.numVertices << endl;
    cout << "header.numTexCoords: " << NewDKM.header.numTexCoords << endl;
    cout << "header.numTriangles: " << NewDKM.header.numTriangles << endl;
    cout << "header.numGlCommands: " << NewDKM.header.numGlCommands << endl;
    cout << "header.numFrames: " << NewDKM.header.numFrames << endl;
    cout << "header.numSurfaces: " << NewDKM.header.numSurfaces << endl;
    cout << "header.offsetSkins: " << NewDKM.header.offsetSkins << endl;
    cout << "header.offsetTexCoords: " << NewDKM.header.offsetTexCoords << endl;
    cout << "header.offsetTriangles: " << NewDKM.header.offsetTriangles << endl;
    cout << "header.offsetFrames: " << NewDKM.header.offsetFrames << endl;
    cout << "header.offsetGlCommands: " << NewDKM.header.offsetGlCommands << endl;
    cout << "header.offsetSurfaces: " << NewDKM.header.offsetSurfaces << endl;
    cout << "header.offsetEnd: " << NewDKM.header.offsetEnd << endl << endl;
    cout << "header.numSequences: " << NewDKM.header.numSequences << endl;
    cout << "header.offsetSequences: " << NewDKM.header.offsetSequences << endl;*/


    inFile.seekg(NewDKM.header.offsetSkins);
    for (int i = 0; i < NewDKM.header.numSkins; i++) {
        char newpath[64];
        inFile.read(newpath, 64);
        NewDKM.skinpaths.emplace_back(newpath);
    }

    inFile.seekg(NewDKM.header.offsetTexCoords);
    for (int i = 0; i < NewDKM.header.numTexCoords; i++) {
        dkm_textureCoordinate_t newtexcord;
        inFile.read(reinterpret_cast<char*>(&newtexcord.s), sizeof(newtexcord.s));
        inFile.read(reinterpret_cast<char*>(&newtexcord.t), sizeof(newtexcord.t));
        NewDKM.UV.push_back(newtexcord);
    }

    inFile.seekg(NewDKM.header.offsetTriangles);
    for (int i = 0; i < NewDKM.header.numTriangles; i++) {
        dkm_triangle_t newtriangle;
        inFile.read(reinterpret_cast<char*>(&newtriangle.surfaceIndex), sizeof(newtriangle.surfaceIndex));
        inFile.read(reinterpret_cast<char*>(&newtriangle.num_uvframes), sizeof(newtriangle.num_uvframes));
        inFile.read(reinterpret_cast<char*>(&newtriangle.vertexIndices[0]), sizeof(newtriangle.vertexIndices[0]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.vertexIndices[1]), sizeof(newtriangle.vertexIndices[1]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.vertexIndices[2]), sizeof(newtriangle.vertexIndices[2]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.textureIndices[0]), sizeof(newtriangle.textureIndices[0]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.textureIndices[1]), sizeof(newtriangle.textureIndices[1]));
        inFile.read(reinterpret_cast<char*>(&newtriangle.textureIndices[2]), sizeof(newtriangle.textureIndices[2]));
        NewDKM.triangles.push_back(newtriangle);
    }
        
    inFile.seekg(NewDKM.header.offsetFrames);
    for (int i = 0; i < NewDKM.header.numFrames; i++) {
        dkm_frame_t newframe;
        inFile.read(reinterpret_cast<char*>(&newframe.scale[0]), sizeof(newframe.scale[0]));
        inFile.read(reinterpret_cast<char*>(&newframe.scale[1]), sizeof(newframe.scale[1]));
        inFile.read(reinterpret_cast<char*>(&newframe.scale[2]), sizeof(newframe.scale[2]));
        inFile.read(reinterpret_cast<char*>(&newframe.translate[0]), sizeof(newframe.translate[0]));
        inFile.read(reinterpret_cast<char*>(&newframe.translate[1]), sizeof(newframe.translate[1]));
        inFile.read(reinterpret_cast<char*>(&newframe.translate[2]), sizeof(newframe.translate[2]));
        char fname[16];
        inFile.read(fname, 16);
        fname[15] = '\0';
        newframe.name = fname;

        for (int j = 0; j < NewDKM.header.numVertices; j++) {
            dkm_triangleVertex_t newvert;
            if (NewDKM.header.version == 1) {
                uint8_t x, y, z;
                inFile.read(reinterpret_cast<char*>(&x), sizeof(x));
                inFile.read(reinterpret_cast<char*>(&y), sizeof(y));
                inFile.read(reinterpret_cast<char*>(&z), sizeof(z));
                newvert.vertex[0] = x;
                newvert.vertex[1] = y;
                newvert.vertex[2] = z;
            }
            else if (NewDKM.header.version == 2) {
                uint32_t packvert;
                inFile.read(reinterpret_cast<char*>(&packvert), sizeof(packvert));
                newvert.vertex[0] = (packvert & 0xFFE00000) >> 21;
                newvert.vertex[1] = (packvert & 0x1FF800) >> 11;
                newvert.vertex[2] = (packvert & 0x7FF);
            }
            inFile.read(reinterpret_cast<char*>(&newvert.lightNormalIndex), sizeof(newvert.lightNormalIndex));
            newframe.vertices.push_back(newvert);
        }
        if (NewDKM.header.version == 2) {
            inFile.read(reinterpret_cast<char*>(&newframe.unknown[0]), sizeof(newframe.unknown[0]));
            inFile.read(reinterpret_cast<char*>(&newframe.unknown[1]), sizeof(newframe.unknown[1]));
            inFile.read(reinterpret_cast<char*>(&newframe.unknown[2]), sizeof(newframe.unknown[2]));
        }
        else {
            newframe.unknown[0] = 0;
            newframe.unknown[1] = 0;
            newframe.unknown[2] = 0;
        }
        NewDKM.frames.push_back(newframe);
    }

    inFile.seekg(NewDKM.header.offsetGlCommands);
    while (true) {
        int command;
        inFile.read(reinterpret_cast<char*>(&command), sizeof(int));
        if (command == 0) break; // End of commands
        dkm_glCommand_t cmd;
        cmd.count = command;
        int numVerts = abs(command);
        inFile.read(reinterpret_cast<char*>(&cmd.skinIndex), sizeof(int));
        inFile.read(reinterpret_cast<char*>(&cmd.surfIndex), sizeof(int));
        for (int i = 0; i < numVerts; i++) {
            dkm_glCommandVertex_t v;
            inFile.read(reinterpret_cast<char*>(&v.vertexIndex), sizeof(v.vertexIndex));
            inFile.read(reinterpret_cast<char*>(&v.s), sizeof(v.s));
            inFile.read(reinterpret_cast<char*>(&v.t), sizeof(v.t));
            cmd.vertices.push_back(v);
        }
        NewDKM.GLCommands.push_back(cmd);
    }

    inFile.seekg(NewDKM.header.offsetSurfaces);
    for (int i = 0; i < NewDKM.header.numSurfaces; i++) {
        dkm_surface_t newsurf;
        char fname[32];
        inFile.read(fname, 32);
        fname[31] = '\0';
        newsurf.name = fname;
        inFile.read(reinterpret_cast<char*>(&newsurf.flags), sizeof(newsurf.flags));
        inFile.read(reinterpret_cast<char*>(&newsurf.skinIndex), sizeof(newsurf.skinIndex));
        inFile.read(reinterpret_cast<char*>(&newsurf.skinWidth), sizeof(newsurf.skinWidth));
        inFile.read(reinterpret_cast<char*>(&newsurf.skinHeight), sizeof(newsurf.skinHeight));
        inFile.read(reinterpret_cast<char*>(&newsurf.numUVframes), sizeof(newsurf.numUVframes));
        NewDKM.surfaces.push_back(newsurf);
        if (newsurf.numUVframes != 1)
            cout << "numUVframes: " << newsurf.numUVframes << endl;
    }

    if (NewDKM.header.numSequences == 0) {
        if (NewDKM.header.offsetSurfaces == inFile.tellg()) {
            cout << "End of file matches header." << endl;
        }
        else {
            cout << "Warning: End of file mismatch with header: " << inFile.tellg() << " vs " << NewDKM.header.offsetEnd << endl;
        }
    }

    if (NewDKM.header.numSequences > 0) {
        inFile.seekg(NewDKM.header.offsetSequences);
        for (int i = 0; i < NewDKM.header.numSequences; i++) {
            dkm_animSeq_t newseq;
            char fname[16];
            inFile.read(fname, 16);
            fname[15] = '\0';
            newseq.name = fname;
            inFile.read(reinterpret_cast<char*>(&newseq.startFrame), sizeof(newseq.startFrame));
            inFile.read(reinterpret_cast<char*>(&newseq.endFrame), sizeof(newseq.endFrame));
            NewDKM.sequences.push_back(newseq);
        }
        
        if (NewDKM.header.offsetEnd == inFile.tellg()) {
            cout << "End of file matches header." << endl;
        }
        else {
            cout << "Warning: End of file mismatch with header: " << inFile.tellg() << " vs " << NewDKM.header.offsetEnd << endl;
        }
    }

    inFile.close();

    return NewDKM;
}

void DKM2JSON(const DKM_file& NewDKM, fs::path outpath){
    json jsonDKM;
    json jheader;
    jheader["ident"] = "DKMD";
    jheader["version"] = NewDKM.header.version;
    jheader["origin"] = { NewDKM.header.origin[0], NewDKM.header.origin[1], NewDKM.header.origin[2] };
    jheader["frameSize"] = NewDKM.header.frameSize;
    jheader["numSkins"] = NewDKM.header.numSkins;
    jheader["numVertices"] = NewDKM.header.numVertices;
    jheader["numTexCoords"] = NewDKM.header.numTexCoords;
    jheader["numTriangles"] = NewDKM.header.numTriangles;
    jheader["numGlCommands"] = NewDKM.header.numGlCommands;
    jheader["numFrames"] = NewDKM.header.numFrames;
    jheader["numSurfaces"] = NewDKM.header.numSurfaces;
    jheader["offsetSkins"] = NewDKM.header.offsetSkins;
    jheader["offsetTexCoords"] = NewDKM.header.offsetTexCoords;
    jheader["offsetTriangles"] = NewDKM.header.offsetTriangles;
    jheader["offsetFrames"] = NewDKM.header.offsetFrames;
    jheader["offsetGlCommands"] = NewDKM.header.offsetGlCommands;
    jheader["offsetSurfaces"] = NewDKM.header.offsetSurfaces;
    jheader["offsetEnd"] = NewDKM.header.offsetEnd;
    jheader["numSequences"] = NewDKM.header.numSequences;
    jheader["offsetSequences"] = NewDKM.header.offsetSequences;
    jsonDKM["header"] = jheader;

    jsonDKM["skins"] = json::array();;
    for (const auto& skinpath : NewDKM.skinpaths) {
        json s;
        s = skinpath;
        jsonDKM["skins"].push_back(s);
    }

    jsonDKM["UV"] = json::array();;
    for (const auto& uve : NewDKM.UV) {
        json s;
        s = { uve.s, uve.t };
        jsonDKM["UV"].push_back(s);
    }

    jsonDKM["triangles"] = json::array();;
    for (const auto& tri : NewDKM.triangles) {
        json s;
        s["surfaceIndex"] = tri.surfaceIndex;
        s["num_uvframes"] = tri.num_uvframes;
        s["vertexIndices"] = { tri.vertexIndices[0], tri.vertexIndices[1], tri.vertexIndices[2] };
        s["textureIndices"] = { tri.textureIndices[0], tri.textureIndices[1], tri.textureIndices[2] };
        jsonDKM["triangles"].push_back(s);
    }

    jsonDKM["frames"] = json::array();
    for (const auto& frame : NewDKM.frames) {
        json jframe;
        jframe["scale"] = { frame.scale[0],frame.scale[1],frame.scale[2] };
        jframe["translate"] = { frame.translate[0], frame.translate[1], frame.translate[2] };
        jframe["name"] = frame.name;
        jframe["verts"] = json::array();
        for (int i = 0; i < NewDKM.header.numVertices; i++) {
            json jvert;
            jvert = { frame.vertices[i].vertex[0], frame.vertices[i].vertex[1], frame.vertices[i].vertex[2], frame.vertices[i].lightNormalIndex };
            jframe["verts"].push_back(jvert);
        }
        jframe["unknown"]= { frame.unknown[0], frame.unknown[1], frame.unknown[2] };
        jsonDKM["frames"].push_back(jframe);
    }

    jsonDKM["glCommands"] = json::array();
    for (const auto& cmd : NewDKM.GLCommands) {
        json jCmd;
        jCmd["strip"] = (cmd.count > 0);
        jCmd["skinIndex"] = cmd.skinIndex;
        jCmd["surfIndex"] = cmd.surfIndex;
        jCmd["verts"] = json::array();
        for (const auto& v : cmd.vertices)
            jCmd["verts"].push_back({ v.vertexIndex, v.s, v.t });
        jsonDKM["glCommands"].push_back(jCmd);
    }

    jsonDKM["surfaces"] = json::array();
    for (const auto& surface : NewDKM.surfaces) {
        json jsurf;
        jsurf["name"] = surface.name;
        jsurf["flags"] = surface.flags;
        jsurf["skinIndex"] = surface.skinIndex;
        jsurf["skinWidth"] = surface.skinWidth;
        jsurf["skinHeight"] = surface.skinHeight;
        jsurf["numUVframes"] = surface.numUVframes;
        jsonDKM["surfaces"].push_back(jsurf);
    }

    jsonDKM["animSeqs"] = json::array();
    for (const auto& as : NewDKM.sequences) {
        json jseq;
        jseq["name"] = as.name;
        jseq["startFrame"] = as.startFrame;
        jseq["endFrame"] = as.endFrame;
        jsonDKM["animSeqs"].push_back(jseq);
    }

    cout << "JSON prepped, time to export." << endl;
    ofstream outFile(outpath);
    if (outFile.is_open()) {
        outFile << jsonDKM.dump(2);
        outFile.close();
        cout << "JSON created successfully: " << outpath << endl;
    }
    else {
        cout << "Error: Could not write output file." << endl;
    }
}
