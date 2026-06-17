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
        
        //new_header.offsetTexCoords = outFile.tellp();
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
        //outFile.write(reinterpret_cast<char*>(&new_header.offsetTexCoords), sizeof(int));
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
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.numTriangles), sizeof(NewMDX.header.numTriangles));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.numGlCommands), sizeof(NewMDX.header.numGlCommands));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.numFrames), sizeof(NewMDX.header.numFrames));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.numSfxDefines), sizeof(NewMDX.header.numSfxDefines));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.numSfxEntries), sizeof(NewMDX.header.numSfxEntries));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.numSubObjects), sizeof(NewMDX.header.numSubObjects));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetSkins), sizeof(NewMDX.header.offsetSkins));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetTriangles), sizeof(NewMDX.header.offsetTriangles));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetFrames), sizeof(NewMDX.header.offsetFrames));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetGlCommands), sizeof(NewMDX.header.offsetGlCommands));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetVertexInfo), sizeof(NewMDX.header.offsetVertexInfo));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetSfxDefines), sizeof(NewMDX.header.offsetSfxDefines));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetSfxEntries), sizeof(NewMDX.header.offsetSfxEntries));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetBBoxFrames), sizeof(NewMDX.header.offsetBBoxFrames));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetDummyEnd), sizeof(NewMDX.header.offsetDummyEnd));
    inFile.read(reinterpret_cast<char*>(&NewMDX.header.offsetEnd), sizeof(NewMDX.header.offsetEnd));

    if (NewMDX.header.offsetSkins < NewMDX.header.offsetTriangles &&
        NewMDX.header.offsetTriangles < NewMDX.header.offsetFrames &&
        NewMDX.header.offsetFrames < NewMDX.header.offsetGlCommands &&
        NewMDX.header.offsetGlCommands < NewMDX.header.offsetVertexInfo &&
        NewMDX.header.offsetVertexInfo < NewMDX.header.offsetSfxDefines &&
        NewMDX.header.offsetSfxDefines < NewMDX.header.offsetSfxEntries &&
        NewMDX.header.offsetSfxEntries < NewMDX.header.offsetBBoxFrames &&
        NewMDX.header.offsetBBoxFrames < NewMDX.header.offsetDummyEnd &&
        NewMDX.header.offsetDummyEnd <= NewMDX.header.offsetEnd) {
        cout << "Offsets are in ascending order." << endl;
    }
    else {
        cout << "WARNING: Offsets are NOT in ascending order." << endl;
        cout << "header.offsetSkins: " << NewMDX.header.offsetSkins << endl;
        cout << "header.offsetTriangles: " << NewMDX.header.offsetTriangles << endl;
        cout << "header.offsetFrames: " << NewMDX.header.offsetFrames << endl;
        cout << "header.offsetGlCommands: " << NewMDX.header.offsetGlCommands << endl;
        cout << "header.offsetVertexInfo: " << NewMDX.header.offsetVertexInfo << endl;
        cout << "header.offsetSfxDefines: " << NewMDX.header.offsetSfxDefines << endl;
        cout << "header.offsetSfxEntries: " << NewMDX.header.offsetSfxEntries << endl;
        cout << "header.offsetBBoxFrames: " << NewMDX.header.offsetBBoxFrames << endl;
        cout << "header.offsetDummyEnd: " << NewMDX.header.offsetDummyEnd << endl;
        cout << "header.offsetEnd: " << NewMDX.header.offsetEnd << endl;
    }

    inFile.seekg(NewMDX.header.offsetSkins);
    for (int i = 0; i < NewMDX.header.numSkins; i++) {
        char newpath[64];
        inFile.read(newpath, 64);
        NewMDX.skinpaths.emplace_back(newpath);
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
        inFile.read(reinterpret_cast<char*>(&cmd.SubObjectID), sizeof(cmd.SubObjectID));
        for (int i = 0; i < numVerts; i++) {
            kp_mdx_glCommandVertex_t v;
            inFile.read(reinterpret_cast<char*>(&v.s), sizeof(float));
            inFile.read(reinterpret_cast<char*>(&v.t), sizeof(float));
            inFile.read(reinterpret_cast<char*>(&v.vertexIndex), sizeof(int));
            cmd.vertices.push_back(v);
        }
        NewMDX.GLCommands.push_back(cmd);
    }

    inFile.seekg(NewMDX.header.offsetVertexInfo);
    for (int i = 0; i < NewMDX.header.numVertices; i++) {
        int newVert;
        inFile.read(reinterpret_cast<char*>(&newVert), sizeof(newVert));
        NewMDX.VertexInfo.push_back(newVert);
    }

    inFile.seekg(NewMDX.header.offsetSfxDefines);
    for (int i = 0; i < NewMDX.header.numSfxDefines; i++) {
        kp_mdx_sfxDefine_t newDef;
        inFile.read(reinterpret_cast<char*>(&newDef.type), sizeof(newDef.type));
        inFile.read(reinterpret_cast<char*>(&newDef.flags), sizeof(newDef.flags));
        inFile.read(reinterpret_cast<char*>(&newDef.velocity_type), sizeof(newDef.velocity_type));
        inFile.read(reinterpret_cast<char*>(&newDef.velocity_speed_up), sizeof(newDef.velocity_speed_up));
        inFile.read(reinterpret_cast<char*>(&newDef.gravity), sizeof(newDef.gravity));
        inFile.read(reinterpret_cast<char*>(&newDef.spawn_interval), sizeof(newDef.spawn_interval));
        inFile.read(reinterpret_cast<char*>(&newDef.random_spawn_interval), sizeof(newDef.random_spawn_interval));
        inFile.read(reinterpret_cast<char*>(&newDef.start_alpha), sizeof(newDef.start_alpha));
        inFile.read(reinterpret_cast<char*>(&newDef.end_alpha), sizeof(newDef.end_alpha));
        inFile.read(reinterpret_cast<char*>(&newDef.fadein_time), sizeof(newDef.fadein_time));
        inFile.read(reinterpret_cast<char*>(&newDef.lifetime), sizeof(newDef.lifetime));
        inFile.read(reinterpret_cast<char*>(&newDef.random_time_scale), sizeof(newDef.random_time_scale));
        inFile.read(reinterpret_cast<char*>(&newDef.start_width), sizeof(newDef.start_width));
        inFile.read(reinterpret_cast<char*>(&newDef.end_width), sizeof(newDef.end_width));
        inFile.read(reinterpret_cast<char*>(&newDef.start_height), sizeof(newDef.start_height));
        inFile.read(reinterpret_cast<char*>(&newDef.end_height), sizeof(newDef.end_height));
        inFile.read(reinterpret_cast<char*>(&newDef.random_size_scale), sizeof(newDef.random_size_scale));
        NewMDX.SfxDefintions.push_back(newDef);
    }

    inFile.seekg(NewMDX.header.offsetSfxEntries);
    for (int i = 0; i < NewMDX.header.numSfxEntries; i++) {
        kp_mdx_sfxEntry_t newEnt;
        inFile.read(reinterpret_cast<char*>(&newEnt.index), sizeof(newEnt.index));
        inFile.read(reinterpret_cast<char*>(&newEnt.define_no), sizeof(newEnt.define_no));
        inFile.read(reinterpret_cast<char*>(&newEnt.vertexindex), sizeof(newEnt.vertexindex));
        for (int j = 0; j < 128; j++)
            inFile.read(reinterpret_cast<char*>(&newEnt.SfxFrames[j]), sizeof(uint8_t));
        NewMDX.SfxEntries.push_back(newEnt);
    }

    inFile.seekg(NewMDX.header.offsetBBoxFrames);
    for (int i = 0; i < NewMDX.header.numSubObjects; i++) {
        kp_mdx_BFrames_t newBF;
        for (int j = 0; j < NewMDX.header.numFrames; j++) {
            kp_mdx_BBox_t newBox;
            inFile.read(reinterpret_cast<char*>(&newBox.MinX), sizeof(newBox.MinX));
            inFile.read(reinterpret_cast<char*>(&newBox.MinY), sizeof(newBox.MinY));
            inFile.read(reinterpret_cast<char*>(&newBox.MinZ), sizeof(newBox.MinZ));
            inFile.read(reinterpret_cast<char*>(&newBox.MaxX), sizeof(newBox.MaxX));
            inFile.read(reinterpret_cast<char*>(&newBox.MaxY), sizeof(newBox.MaxY));
            inFile.read(reinterpret_cast<char*>(&newBox.MaxZ), sizeof(newBox.MaxZ));
            newBF.BoxFrames.push_back(newBox);
        }
        NewMDX.BBoxFrames.push_back(newBF);
    }

    if (NewMDX.header.offsetDummyEnd == inFile.tellg()) cout << "End of file matches header Dummy." << endl;
    else cout << "Warning: End of file mismatch with header Dummy: " << inFile.tellg() << " vs " << NewMDX.header.offsetDummyEnd << endl;
    if (NewMDX.header.offsetEnd == inFile.tellg()) cout << "End of file matches header." << endl;
    else cout << "Warning: End of file mismatch with header: " << inFile.tellg() << " vs " << NewMDX.header.offsetEnd << endl;

    inFile.close();

    return NewMDX;
}

void KPMDX2JSON(const kp_mdx_file& NewMDX, fs::path outpath) {
    json jsonMDX;
    json jheader;
    jheader["ident"] = "IDPX";
    jheader["version"] = NewMDX.header.version;
    jheader["skinWidth"] = NewMDX.header.skinWidth;
    jheader["skinHeight"] = NewMDX.header.skinHeight;
    jheader["frameSize"] = NewMDX.header.frameSize;
    jheader["numSkins"] = NewMDX.header.numSkins;
    jheader["numVertices"] = NewMDX.header.numVertices;
    jheader["numTriangles"] = NewMDX.header.numTriangles;
    jheader["numGlCommands"] = NewMDX.header.numGlCommands;
    jheader["numFrames"] = NewMDX.header.numFrames;
    jheader["numSfxDefines"] = NewMDX.header.numSfxDefines;
    jheader["numSfxEntries"] = NewMDX.header.numSfxEntries;
    jheader["numSubObjects"] = NewMDX.header.numSubObjects;
    jheader["offsetSkins"] = NewMDX.header.offsetSkins;
    jheader["offsetTriangles"] = NewMDX.header.offsetTriangles;
    jheader["offsetFrames"] = NewMDX.header.offsetFrames;
    jheader["offsetGlCommands"] = NewMDX.header.offsetGlCommands;
    jheader["offsetVertexInfo"] = NewMDX.header.offsetVertexInfo;
    jheader["offsetSfxDefines"] = NewMDX.header.offsetSfxDefines;
    jheader["offsetSfxEntries"] = NewMDX.header.offsetSfxEntries;
    jheader["offsetBBoxFrames"] = NewMDX.header.offsetBBoxFrames;
    jheader["offsetDummyEnd"] = NewMDX.header.offsetDummyEnd;
    jheader["offsetEnd"] = NewMDX.header.offsetEnd;
    jsonMDX["header"] = jheader;

    jsonMDX["skins"] = json::array();;
    for (const auto& skinpath : NewMDX.skinpaths) {
        json s;
        s = skinpath;
        jsonMDX["skins"].push_back(s);
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
        jCmd["SubObjectID"] = cmd.SubObjectID;
        jCmd["verts"] = json::array();
        for (const auto& v : cmd.vertices)
            jCmd["verts"].push_back({ v.s, v.t, v.vertexIndex });
        jsonMDX["glCommands"].push_back(jCmd);
    }

    jsonMDX["VertexInfo"] = json::array();
    for (const auto& vi : NewMDX.VertexInfo) {
        json s;
        s = { vi };
        jsonMDX["VertexInfo"].push_back(s);
    }

    jsonMDX["SfxDefintions"] = json::array();
    for (const auto& sfd : NewMDX.SfxDefintions) {
        json jsd;
        jsd["type"] = sfd.type;
        jsd["flags"] = sfd.flags;
        jsd["velocity_type"] = sfd.velocity_type;
        jsd["velocity_speed_up"] = sfd.velocity_speed_up;
        jsd["gravity"] = sfd.gravity;
        jsd["spawn_interval"] = sfd.spawn_interval;
        jsd["random_spawn_interval"] = sfd.random_spawn_interval;
        jsd["start_alpha"] = sfd.start_alpha;
        jsd["end_alpha"] = sfd.end_alpha;
        jsd["fadein_time"] = sfd.fadein_time;
        jsd["lifetime"] = sfd.lifetime;
        jsd["random_time_scale"] = sfd.random_time_scale;
        jsd["start_width"] = sfd.start_width;
        jsd["end_width"] = sfd.end_width;
        jsd["start_height"] = sfd.start_height;
        jsd["end_height"] = sfd.end_height;
        jsd["random_size_scale"] = sfd.random_size_scale;
        jsonMDX["SfxDefintions"].push_back(jsd);
    }

    jsonMDX["SfxEntries"] = json::array();
    for (const auto& sfe : NewMDX.SfxEntries) {
        json jse;
        jse["index"] = sfe.index;
        jse["define_no"] = sfe.define_no;
        jse["vertexindex"] = sfe.vertexindex;
        jse["SfxFrames"] = json::array();
        for (const auto& f : sfe.SfxFrames)
            jse["SfxFrames"].push_back(f);
        jsonMDX["SfxEntries"].push_back(jse);
    }

    jsonMDX["BBoxFrames"] = json::array();
    for (const auto& bbf : NewMDX.BBoxFrames) {
        json jbf = json::array();
        for (const auto& bfr : bbf.BoxFrames) {
            json jbb;
            jbf.push_back({ bfr.MinX, bfr.MinY, bfr.MinZ, bfr.MaxX, bfr.MaxY, bfr.MaxZ });
        }
        jsonMDX["SfxEntries"].push_back(jbf);
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