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

void JSON2DKM(fs::path inpath, fs::path outpath, json jsonDKM) {
    try {
        ofstream outFile(outpath, ios::binary);
        if (!outFile.is_open()) {
            cout << "Error: Could not open output file." << endl;
            return;
        }

        dkm_header_t new_header;

        auto jheader = jsonDKM.at("header");
        new_header.version = jheader["version"];
        new_header.origin[0] = jheader["origin"][0];
        new_header.origin[1] = jheader["origin"][1];
        new_header.origin[2] = jheader["origin"][2];
        
        new_header.numSkins = jsonDKM["skins"].size();
        new_header.numTexCoords = jsonDKM["UV"].size();
        new_header.numTriangles = jsonDKM["triangles"].size();
        new_header.numFrames = jsonDKM["frames"].size();
        new_header.numSurfaces = jsonDKM["surfaces"].size();
        new_header.numSequences = jsonDKM["animSeqs"].size();

        new_header.numVertices = -1;
        for (const auto& f : jsonDKM["frames"]) {
            if (new_header.numVertices == -1)
                new_header.numVertices = f["verts"].size();
            else if (new_header.numVertices != f["verts"].size()) {
                cout << "Error: Frame " << (f["name"]) << " has " << f["verts"].size() << " verts, but expected " << new_header.numVertices << endl;
                return;
            }
        }

        new_header.numGlCommands = 0;
        for (const auto& g : jsonDKM["glCommands"]) {
            new_header.numGlCommands += 3;
            new_header.numGlCommands += (g["verts"].size() * 3);
        }
        new_header.numGlCommands++;

        if (new_header.version == 1) {
            new_header.frameSize = new_header.numVertices * 4 + 40;
        }
        else if (new_header.version == 2) {
            new_header.frameSize = new_header.numVertices * 5 + 43;
        }
        
        outFile.write("DKMD", 4);
        outFile.write(reinterpret_cast<char*>(&new_header.version), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.origin[0]), sizeof(new_header.origin[0]));
        outFile.write(reinterpret_cast<char*>(&new_header.origin[1]), sizeof(new_header.origin[1]));
        outFile.write(reinterpret_cast<char*>(&new_header.origin[2]), sizeof(new_header.origin[2]));
        outFile.write(reinterpret_cast<char*>(&new_header.frameSize), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numSkins), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numVertices), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numTexCoords), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numTriangles), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numGlCommands), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numFrames), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.numSurfaces), sizeof(int));
        for (int i = 0; i < 28; i++) outFile.write("X", 1);
        
        if (new_header.numSequences > 0) {
            outFile.write(reinterpret_cast<char*>(&new_header.numSequences), sizeof(int));
            for (int i = 0; i < 4; i++) outFile.write("X", 1);
            cout << "Animation sequence data block is present." << endl;
        }
        else
            cout << "Animation sequence data block is absent." << endl;
        
        new_header.offsetSkins = outFile.tellp();
        for (const auto& s : jsonDKM["skins"]) {
            char name[64] = { 0 };
            string sName = s;
            strncpy(name, sName.c_str(), 63);
            outFile.write(name, 64);
        }
        
        new_header.offsetTexCoords = outFile.tellp();
        for (const auto& uv : jsonDKM["UV"]) {
            short s = uv[0];
            short t = uv[1];
            outFile.write(reinterpret_cast<char*>(&s), sizeof(s));
            outFile.write(reinterpret_cast<char*>(&t), sizeof(t));
        }
        
        new_header.offsetTriangles = outFile.tellp();
        for (const auto& tri : jsonDKM["triangles"]) {
            short surfaceIndex = tri["surfaceIndex"];
            short num_uvframes = tri["num_uvframes"];
            short vertexIndices[3] = { tri["vertexIndices"][0], tri["vertexIndices"][1], tri["vertexIndices"][2] };
            short textureIndices[3] = { tri["textureIndices"][0], tri["textureIndices"][1], tri["textureIndices"][2] };
            outFile.write(reinterpret_cast<char*>(&surfaceIndex), sizeof(surfaceIndex));
            outFile.write(reinterpret_cast<char*>(&num_uvframes), sizeof(num_uvframes));
            outFile.write(reinterpret_cast<char*>(&vertexIndices), sizeof(vertexIndices));
            outFile.write(reinterpret_cast<char*>(&textureIndices), sizeof(textureIndices));
        }
        
        new_header.offsetFrames = outFile.tellp();
        for (const auto& f : jsonDKM["frames"]) {
            float scale[3] = { f["scale"][0], f["scale"][1], f["scale"][2]};
            float translate[3] = { f["translate"][0], f["translate"][1], f["translate"][2] };
            char name[16] = { 0 };
            string sName = f["name"];
            strncpy(name, sName.c_str(), 15);

            outFile.write(reinterpret_cast<char*>(&scale), sizeof(scale));
            outFile.write(reinterpret_cast<char*>(&translate), sizeof(translate));
            outFile.write(name, 16);

            for (const auto& v : f["verts"]) {
                if (new_header.version == 1) {
                    byte x = v[0];
                    byte y = v[1];
                    byte z = v[2];
                    outFile.write(reinterpret_cast<char*>(&x), sizeof(x));
                    outFile.write(reinterpret_cast<char*>(&y), sizeof(y));
                    outFile.write(reinterpret_cast<char*>(&z), sizeof(z));
                }
                else if (new_header.version == 2) {
                    uint32_t x = static_cast<uint32_t>(v[0]);
                    uint32_t y = static_cast<uint32_t>(v[1]);
                    uint32_t z = static_cast<uint32_t>(v[2]);
                    uint32_t packvert = ((x & 0x7FF) << 21) | ((y & 0x3FF) << 11) | ((z & 0x7FF));
                    outFile.write(reinterpret_cast<char*>(&packvert), sizeof(packvert));
                }
                byte lightNormalIndex = v[3];
                outFile.write(reinterpret_cast<char*>(&lightNormalIndex), sizeof(lightNormalIndex));
            }
            if (new_header.version == 2) {
                byte unknown[3] = { f["unknown"][0], f["unknown"][1], f["unknown"][2] };
                outFile.write(reinterpret_cast<char*>(&unknown), sizeof(unknown));
            }
        }
        
        new_header.offsetGlCommands = outFile.tellp();
        for (const auto& g : jsonDKM["glCommands"]) {
            int vcount = g["verts"].size();
            if (g["strip"].get<bool>() == false) vcount *= (-1);
            outFile.write(reinterpret_cast<char*>(&vcount), sizeof(vcount));
            int skinIndex = g["skinIndex"];
            outFile.write(reinterpret_cast<char*>(&skinIndex), sizeof(skinIndex));
            int surfIndex = g["surfIndex"];
            outFile.write(reinterpret_cast<char*>(&surfIndex), sizeof(surfIndex));
            for (const auto& v : g["verts"]) {
                int vertexIndex = v[0];
                float vs = v[1];
                float vt = v[2];
                outFile.write(reinterpret_cast<char*>(&vertexIndex), sizeof(vertexIndex));
                outFile.write(reinterpret_cast<char*>(&vs), sizeof(vs));
                outFile.write(reinterpret_cast<char*>(&vt), sizeof(vt));
            }
        }
        outFile.write("\0\0\0\0", 4);

        new_header.offsetSurfaces = outFile.tellp();
        for (const auto& s : jsonDKM["surfaces"]) {
            char name[32] = { 0 };
            string sName = s["name"];
            strncpy(name, sName.c_str(), 31);
            int flags = s["flags"];
            int skinIndex = s["skinIndex"];
            int skinWidth = s["skinWidth"];
            int skinHeight = s["skinHeight"];
            int numUVframes = s["numUVframes"];
            outFile.write(name, 32);
            outFile.write(reinterpret_cast<char*>(&flags), sizeof(flags));
            outFile.write(reinterpret_cast<char*>(&skinIndex), sizeof(skinIndex));
            outFile.write(reinterpret_cast<char*>(&skinWidth), sizeof(skinWidth));
            outFile.write(reinterpret_cast<char*>(&skinHeight), sizeof(skinHeight));
            outFile.write(reinterpret_cast<char*>(&numUVframes), sizeof(numUVframes));
        }
        
        if (new_header.numSequences > 0) {
            new_header.offsetSequences = outFile.tellp();
            for (const auto& as : jsonDKM["animSeqs"]) {
                char name[16] = { 0 };
                string sName = as["name"];
                strncpy(name, sName.c_str(), 15);
                int startFrame = as["startFrame"];
                int endFrame = as["endFrame"];
                outFile.write(name, 16);
                outFile.write(reinterpret_cast<char*>(&startFrame), sizeof(startFrame));
                outFile.write(reinterpret_cast<char*>(&endFrame), sizeof(endFrame));
            }
        }
        
        new_header.offsetEnd = outFile.tellp();

        outFile.seekp(52);
        outFile.write(reinterpret_cast<char*>(&new_header.offsetSkins), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetTexCoords), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetTriangles), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetFrames), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetGlCommands), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetSurfaces), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.offsetEnd), sizeof(int));
        if (new_header.numSequences > 0) {
            outFile.write(reinterpret_cast<char*>(&new_header.numSequences), sizeof(int));
            outFile.write(reinterpret_cast<char*>(&new_header.offsetSequences), sizeof(int));
        }

        outFile.close();
        cout << "DKM constructed successfully: " << outpath << endl;

    }
    catch (exception& e) {
        cout << "JSON Parsing Error: " << e.what() << endl;
    }
}

DKM_file ParseDKM(fs::path inpath) {
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
