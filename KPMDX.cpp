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

KP_MDX_file JSON2KPMDX(fs::path inpath, json jsonMDX) {
    try {
        KP_MDX_file newMDX;
        newMDX.header.magic = 1481655369;//IDPX
        newMDX.header.version = 4;
        newMDX.header.numSkins = jsonMDX["skins"].size();
        newMDX.header.numTriangles = jsonMDX["triangles"].size();
        newMDX.header.numFrames = jsonMDX["frames"].size();
        newMDX.header.numSfxDefines = jsonMDX["SfxDefintions"].size();
        newMDX.header.numSfxEntries = jsonMDX["SfxEntries"].size();
        newMDX.header.numSubObjects = jsonMDX["BBoxFrames"].size();

        newMDX.header.numVertices = -1;
        for (const auto& f : jsonMDX["frames"]) {
            if (newMDX.header.numVertices == -1)
                newMDX.header.numVertices = f["verts"].size();
            else if (newMDX.header.numVertices != f["verts"].size()) {
                cout << "Error: Frame " << (f["name"]) << " has " << f["verts"].size() << " verts, but expected " << newMDX.header.numVertices << endl;
                exit(1);
            }
        }

        newMDX.header.numGlCommands = 0;
        for (const auto& g : jsonMDX["glCommands"]) {
            newMDX.header.numGlCommands += 2;
            newMDX.header.numGlCommands += (g["verts"].size() * 3);
        }
        newMDX.header.numGlCommands++;

        

        auto jheader = jsonMDX.at("header");
        newMDX.header.skinWidth = jheader["skinWidth"];
        newMDX.header.skinHeight = jheader["skinHeight"];
        newMDX.header.HDModel = jheader["HDModel"];
        
        if(newMDX.header.HDModel)
            newMDX.header.frameSize = newMDX.header.numVertices * 7 + 40;
        else
            newMDX.header.frameSize = newMDX.header.numVertices * 4 + 40;

        newMDX.header.offsetSkins = 92;
        for (const auto& s : jsonMDX["skins"])
            newMDX.skinpaths.push_back(s);
        
        newMDX.header.offsetTriangles = newMDX.header.offsetSkins + newMDX.header.numSkins * 64;
        for (auto& tri : jsonMDX["triangles"]) {
            kp_mdx_triangle_t newTri;
            newTri.vertexIndices[0] = tri[0];
            newTri.vertexIndices[1] = tri[1];
            newTri.vertexIndices[2] = tri[2];
            newTri.textureIndices[0] = tri[3];
            newTri.textureIndices[1] = tri[4];
            newTri.textureIndices[2] = tri[5];
            newMDX.triangles.push_back(newTri);
        }
        
        newMDX.header.offsetFrames = newMDX.header.offsetTriangles + newMDX.header.numTriangles * 12;
        for (const auto& f : jsonMDX["frames"]) {
            kp_mdx_frame_t newFrame;
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
                kp_mdx_triangleVertex_t newVert;
                newVert.vertex[0] = v[0];
                newVert.vertex[1] = v[1];
                newVert.vertex[2] = v[2];
                newVert.lightNormalIndex = v[3];
                newFrame.vertices.push_back(newVert);
            }
            if (newMDX.header.HDModel) {
                for (const auto& v : f["HDverts"]) {
                    kp_mdx_HDtriangleVertex_t newVert;
                    newVert.vertex[0] = v[0];
                    newVert.vertex[1] = v[1];
                    newVert.vertex[2] = v[2];
                    newFrame.HDvertices.push_back(newVert);
                }
            }
            newMDX.frames.push_back(newFrame);
        }
        
        newMDX.header.offsetGlCommands = newMDX.header.offsetFrames + newMDX.header.numFrames * newMDX.header.frameSize;
        for (const auto& g : jsonMDX["glCommands"]) {
            kp_mdx_glCommand_t newGLC;
            newGLC.count = g["verts"].size();
            if (g["strip"].get<bool>() == false) newGLC.count *= (-1);
            newGLC.SubObjectID = g["SubObjectID"];
            for (const auto& v : g["verts"]) {
                kp_mdx_glCommandVertex_t newGLV;
                newGLV.s = v[0];
                newGLV.t = v[1];
                newGLV.vertexIndex = v[2];
                newGLC.vertices.push_back(newGLV);
            }
            newMDX.GLCommands.push_back(newGLC);
        }

        newMDX.header.offsetVertexInfo = newMDX.header.offsetGlCommands + newMDX.header.numGlCommands * 4;
        for (const auto& vi : jsonMDX["VertexInfo"])
            newMDX.VertexInfo.push_back(vi);
        
        newMDX.header.offsetSfxDefines = newMDX.header.offsetVertexInfo + newMDX.header.numVertices * 4;
        for (const auto& sd : jsonMDX["SfxDefintions"]) {
            kp_mdx_sfxDefine_t newSD;
            newSD.type = sd["type"];
            newSD.flags = sd["flags"];
            newSD.velocity_type = sd["velocity_type"];
            newSD.velocity_speed_up = sd["velocity_speed_up"];
            newSD.gravity = sd["gravity"];
            newSD.spawn_interval = sd["spawn_interval"];
            newSD.random_spawn_interval = sd["random_spawn_interval"];
            newSD.start_alpha = sd["start_alpha"];
            newSD.end_alpha = sd["end_alpha"];
            newSD.fadein_time = sd["fadein_time"];
            newSD.lifetime = sd["lifetime"];
            newSD.random_time_scale = sd["random_time_scale"];
            newSD.start_width = sd["start_width"];
            newSD.end_width = sd["end_width"];
            newSD.start_height = sd["start_height"];
            newSD.end_height = sd["end_height"];
            newSD.random_size_scale = sd["random_size_scale"];
            newMDX.SfxDefintions.push_back(newSD);
        }
        
        newMDX.header.offsetSfxEntries = newMDX.header.offsetSfxDefines + newMDX.header.numSfxDefines * sizeof(kp_mdx_sfxDefine_t);
        for (const auto& se : jsonMDX["SfxEntries"]) {
            kp_mdx_sfxEntry_t newSE;
            newSE.index = se["index"];
            newSE.define_no = se["define_no"];
            newSE.vertexindex = se["vertexindex"];
            for (int i = 0; i < 128; i++)
                newSE.SfxFrames[i] = se["SfxFrames"][i];
            newMDX.SfxEntries.push_back(newSE);
        }

        newMDX.header.offsetBBoxFrames = newMDX.header.offsetSfxEntries + newMDX.header.numSfxEntries * sizeof(kp_mdx_sfxEntry_t);
        for (const auto& bf : jsonMDX["BBoxFrames"]) {
            kp_mdx_BFrames_t newBF;
            for (const auto& bbox : bf) {
                kp_mdx_BBox_t newbox;
                newbox.MinX = bbox[0];
                newbox.MinY = bbox[1];
                newbox.MinZ = bbox[2];
                newbox.MaxX = bbox[3];
                newbox.MaxY = bbox[4];
                newbox.MaxZ = bbox[5];
                newBF.BoxFrames.push_back(newbox);
            }
            newMDX.BBoxFrames.push_back(newBF);
        }
        
        newMDX.header.offsetDummyEnd = newMDX.header.offsetBBoxFrames + newMDX.header.numSubObjects * newMDX.header.numFrames * 24;
        newMDX.header.offsetEnd = newMDX.header.offsetDummyEnd;
        cout << "JSON parsed, creating MDX..." << endl;
        return newMDX;
    }
    catch (exception& e) {
        cout << "JSON Parsing Error: " << e.what() << endl;
        exit(1);
    }
}

void WriteKPMDX(fs::path outpath, KP_MDX_file newMDX) {
    try {
        ofstream outFile(outpath, ios::binary);
        if (!outFile.is_open()) {
            cout << "Error: Could not open output file." << endl;
            return;
        }

        outFile.write("IDPX", 4);
        outFile.write(reinterpret_cast<char*>(&newMDX.header.version), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.skinWidth), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.skinHeight), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.frameSize), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.numSkins), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.numVertices), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.numTriangles), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.numGlCommands), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.numFrames), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.numSfxDefines), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.numSfxEntries), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.numSubObjects), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.offsetSkins), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.offsetTriangles), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.offsetFrames), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.offsetGlCommands), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.offsetVertexInfo), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.offsetSfxDefines), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.offsetSfxEntries), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.offsetBBoxFrames), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.offsetDummyEnd), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&newMDX.header.offsetEnd), sizeof(int));

        for (const auto& s : newMDX.skinpaths) {
            char name[64] = { 0 };
            strncpy(name, s.c_str(), 63);
            outFile.write(name, 64);
        }

        for (auto& tri : newMDX.triangles)
            outFile.write(reinterpret_cast<char*>(&tri), sizeof(tri));

        for (auto& f : newMDX.frames) {
            outFile.write(reinterpret_cast<char*>(&f.scale), sizeof(f.scale));
            outFile.write(reinterpret_cast<char*>(&f.translate), sizeof(f.translate));
            outFile.write(f.name, 16);
            for (auto& v : f.vertices)
                outFile.write(reinterpret_cast<char*>(&v), sizeof(v));
            if (newMDX.header.HDModel)
                for (auto& v : f.HDvertices)
                    outFile.write(reinterpret_cast<char*>(&v), sizeof(v));
        }

        for (auto& g : newMDX.GLCommands) {
            outFile.write(reinterpret_cast<char*>(&g.count), sizeof(g.count));
            outFile.write(reinterpret_cast<char*>(&g.SubObjectID), sizeof(g.SubObjectID));
            for (auto& v : g.vertices) {
                outFile.write(reinterpret_cast<char*>(&v.s), sizeof(v.s));
                outFile.write(reinterpret_cast<char*>(&v.t), sizeof(v.t));
                outFile.write(reinterpret_cast<char*>(&v.vertexIndex), sizeof(v.vertexIndex));
            }
        }
        outFile.write("\0\0\0\0", 4);

        for (auto& vi : newMDX.VertexInfo)
            outFile.write(reinterpret_cast<char*>(&vi), sizeof(vi));

        for (auto& newSD : newMDX.SfxDefintions) {
            outFile.write(reinterpret_cast<char*>(&newSD.type), sizeof(newSD.type));
            outFile.write(reinterpret_cast<char*>(&newSD.flags), sizeof(newSD.flags));
            outFile.write(reinterpret_cast<char*>(&newSD.velocity_type), sizeof(newSD.velocity_type));
            outFile.write(reinterpret_cast<char*>(&newSD.velocity_speed_up), sizeof(newSD.velocity_speed_up));
            outFile.write(reinterpret_cast<char*>(&newSD.gravity), sizeof(newSD.gravity));
            outFile.write(reinterpret_cast<char*>(&newSD.spawn_interval), sizeof(newSD.spawn_interval));
            outFile.write(reinterpret_cast<char*>(&newSD.random_spawn_interval), sizeof(newSD.random_spawn_interval));
            outFile.write(reinterpret_cast<char*>(&newSD.start_alpha), sizeof(newSD.start_alpha));
            outFile.write(reinterpret_cast<char*>(&newSD.end_alpha), sizeof(newSD.end_alpha));
            outFile.write(reinterpret_cast<char*>(&newSD.fadein_time), sizeof(newSD.fadein_time));
            outFile.write(reinterpret_cast<char*>(&newSD.lifetime), sizeof(newSD.lifetime));
            outFile.write(reinterpret_cast<char*>(&newSD.random_time_scale), sizeof(newSD.random_time_scale));
            outFile.write(reinterpret_cast<char*>(&newSD.start_width), sizeof(newSD.start_width));
            outFile.write(reinterpret_cast<char*>(&newSD.end_width), sizeof(newSD.end_width));
            outFile.write(reinterpret_cast<char*>(&newSD.start_height), sizeof(newSD.start_height));
            outFile.write(reinterpret_cast<char*>(&newSD.end_height), sizeof(newSD.end_height));
            outFile.write(reinterpret_cast<char*>(&newSD.random_size_scale), sizeof(newSD.random_size_scale));
        }

        for (auto& newSE : newMDX.SfxEntries) {
            outFile.write(reinterpret_cast<char*>(&newSE.index), sizeof(newSE.index));
            outFile.write(reinterpret_cast<char*>(&newSE.define_no), sizeof(newSE.define_no));
            outFile.write(reinterpret_cast<char*>(&newSE.vertexindex), sizeof(newSE.vertexindex));
            for (auto& f : newSE.SfxFrames)
                outFile.write(reinterpret_cast<char*>(&f), 1);
        }

        for (auto& bf : newMDX.BBoxFrames) {
            for (auto& newbox : bf.BoxFrames) {
                outFile.write(reinterpret_cast<char*>(&newbox.MinX), sizeof(newbox.MinX));
                outFile.write(reinterpret_cast<char*>(&newbox.MinY), sizeof(newbox.MinY));
                outFile.write(reinterpret_cast<char*>(&newbox.MinZ), sizeof(newbox.MinZ));
                outFile.write(reinterpret_cast<char*>(&newbox.MaxX), sizeof(newbox.MaxX));
                outFile.write(reinterpret_cast<char*>(&newbox.MaxY), sizeof(newbox.MaxY));
                outFile.write(reinterpret_cast<char*>(&newbox.MaxZ), sizeof(newbox.MaxZ));
            }
        }
        if (newMDX.header.offsetEnd != outFile.tellp())
            cout << "Warning: offsetEnd doesn't match the end of the file: " << newMDX.header.offsetEnd << " vs " << outFile.tellp() << endl;
        outFile.close();
        cout << "MDX constructed successfully: " << outpath << endl;
    }
    catch (exception& e) {
        cout << "Error: " << e.what() << endl;
    }
}

KP_MDX_file ReadKPMDX(fs::path inpath) {
    ifstream inFile(inpath, ios::binary);
    KP_MDX_file NewMDX;

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

    if (NewMDX.header.frameSize == 40 + NewMDX.header.numVertices * 7) {
        NewMDX.header.HDModel = true;
        cout << "Model is HD (16-bit vertices)." << endl;
    }
    else
        cout << "Model is SD (8-bit vertices)." << endl;
        

    if (NewMDX.header.offsetSkins < NewMDX.header.offsetTriangles &&
        NewMDX.header.offsetTriangles < NewMDX.header.offsetFrames &&
        NewMDX.header.offsetFrames < NewMDX.header.offsetGlCommands &&
        NewMDX.header.offsetGlCommands < NewMDX.header.offsetVertexInfo &&
        NewMDX.header.offsetVertexInfo <= NewMDX.header.offsetSfxDefines &&
        NewMDX.header.offsetSfxDefines <= NewMDX.header.offsetSfxEntries &&
        NewMDX.header.offsetSfxEntries <= NewMDX.header.offsetBBoxFrames &&
        NewMDX.header.offsetBBoxFrames <= NewMDX.header.offsetDummyEnd &&
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

    if (NewMDX.header.numSfxDefines)cout << "SFX Definitions present: " << NewMDX.header.numSfxDefines << endl;
    if (NewMDX.header.numSfxEntries)cout << "SFX Entries present: " << NewMDX.header.numSfxEntries << endl;

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
        if (NewMDX.header.HDModel) {
            for (int j = 0; j < NewMDX.header.numVertices; j++) {
                kp_mdx_HDtriangleVertex_t newvert;
                inFile.read(reinterpret_cast<char*>(&newvert.vertex[0]), sizeof(newvert.vertex[0]));
                inFile.read(reinterpret_cast<char*>(&newvert.vertex[1]), sizeof(newvert.vertex[1]));
                inFile.read(reinterpret_cast<char*>(&newvert.vertex[2]), sizeof(newvert.vertex[2]));
                newframe.HDvertices.push_back(newvert);
            }
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

void KPMDX2JSON(const KP_MDX_file& NewMDX, fs::path outpath) {
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
    jheader["HDModel"] = NewMDX.header.HDModel;
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
        if (NewMDX.header.HDModel) {
            jframe["HDverts"] = json::array();
            for (int i = 0; i < NewMDX.header.numVertices; i++) {
                json jvert;
                jvert = { frame.HDvertices[i].vertex[0],frame.HDvertices[i].vertex[1], frame.HDvertices[i].vertex[2] };
                jframe["HDverts"].push_back(jvert);
            }
        }
        else
            jframe["HDverts"] = nullptr;
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
        s = vi;
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
        jsonMDX["BBoxFrames"].push_back(jbf);
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