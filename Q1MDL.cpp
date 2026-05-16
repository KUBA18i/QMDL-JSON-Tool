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

using namespace std;
using json = nlohmann::ordered_json;
namespace fs = filesystem;

void JSON2Q1MDL(fs::path inpath, fs::path outpath, json jsonMDL) {
    try {
        ofstream outFile(outpath, ios::binary);
        if (!outFile.is_open()) {
            cout << "Error: Could not open output file." << endl;
            return;
        }

        q1_mdl_header_t new_header;
        new_header.numSkins = jsonMDL["skins"].size();
        new_header.numTris = jsonMDL["triangles"].size();
        new_header.numFrames = jsonMDL["frames"].size();
        
        new_header.numVerts = -1;
        for (const auto& f : jsonMDL["frames"]) {
            if (f["group"].get<bool>() == false) {
                auto& sframe = f["frame"][0];
                if (new_header.numVerts == -1)
                    new_header.numVerts = sframe["verts"].size();
                else if (new_header.numVerts != sframe["verts"].size()) {
                    cout << "Error: Frame " << (f["frame"][0]["name"]) << " has " << sframe["verts"].size() << " verts, but expected " << new_header.numVerts << endl;
                    return;
                }
            }
            else {
                for (const auto& gframe : f["frames"]) {
                    if (new_header.numVerts == -1)
                        new_header.numVerts = gframe["verts"].size();
                    else if (gframe["verts"].size() != new_header.numVerts) {
                        cout << "Error: Subframe " << gframe["name"] << " in group has " << gframe["verts"].size() << " verts, but expected " << new_header.numVerts << endl;
                        return;
                    }
                }
            }
        }

        outFile.write("IDPO", 4);
        new_header.version = 6;
        outFile.write(reinterpret_cast<const char*>(&new_header.version), sizeof(new_header.version));

        auto jheader = jsonMDL.at("header");
        new_header.scale[0] = jheader["scale"][0];
        new_header.scale[1] = jheader["scale"][1];
        new_header.scale[2] = jheader["scale"][2];
        new_header.translate[0] = jheader["translate"][0];
        new_header.translate[1] = jheader["translate"][1];
        new_header.translate[2] = jheader["translate"][2];
        new_header.boundingRadius = jheader["boundingRadius"];
        new_header.eyePosition[0] = jheader["eyePosition"][0];
        new_header.eyePosition[1] = jheader["eyePosition"][1];
        new_header.eyePosition[2] = jheader["eyePosition"][2];

        outFile.write(reinterpret_cast<char*>(&new_header.scale), sizeof(new_header.scale));
        outFile.write(reinterpret_cast<char*>(&new_header.translate), sizeof(new_header.translate));
        outFile.write(reinterpret_cast<char*>(&new_header.boundingRadius), sizeof(new_header.boundingRadius));
        outFile.write(reinterpret_cast<char*>(&new_header.eyePosition), sizeof(new_header.eyePosition));

        new_header.skinWidth = jheader["skinWidth"];
        new_header.skinHeight = jheader["skinHeight"];
        new_header.syncType = jheader["syncType"];
        new_header.flags = jheader["flags"];
        new_header.size = jheader["size"];

        outFile.write(reinterpret_cast<char*>(&new_header.numSkins), sizeof(new_header.numSkins));
        outFile.write(reinterpret_cast<char*>(&new_header.skinWidth), sizeof(new_header.skinWidth));
        outFile.write(reinterpret_cast<char*>(&new_header.skinHeight), sizeof(new_header.skinHeight));
        outFile.write(reinterpret_cast<char*>(&new_header.numVerts), sizeof(new_header.numVerts));
        outFile.write(reinterpret_cast<char*>(&new_header.numTris), sizeof(new_header.numTris));
        outFile.write(reinterpret_cast<char*>(&new_header.numFrames), sizeof(new_header.numFrames));
        outFile.write(reinterpret_cast<char*>(&new_header.syncType), sizeof(new_header.syncType));
        outFile.write(reinterpret_cast<char*>(&new_header.flags), sizeof(new_header.flags));
        outFile.write(reinterpret_cast<char*>(&new_header.size), sizeof(new_header.size));

        for (const auto& s : jsonMDL["skins"]) {
            int group = s["group"].get<bool>() ? 1 : 0;
            outFile.write(reinterpret_cast<char*>(&group), sizeof(group));
            if (group == 0) {
                vector<uint8_t> data = s["skin"].get<vector<uint8_t>>();
                outFile.write(reinterpret_cast<char*>(data.data()), data.size());
            }
            else {
                int nb = s["nb"];
                outFile.write(reinterpret_cast<char*>(&nb), sizeof(nb));
                vector<float> times = s["time"].get<vector<float>>();
                outFile.write(reinterpret_cast<char*>(times.data()), times.size() * sizeof(float));
                for (const auto& frameData : s["skins"]) {
                    vector<uint8_t> data = frameData.get<vector<uint8_t>>();
                    outFile.write(reinterpret_cast<char*>(data.data()), data.size());
                }
            }
        }

        for (const auto& uv : jsonMDL["UV"]) {
            int onseam = uv[0].get<bool>() ? 32 : 0;
            int s = uv[1];
            int t = uv[2];
            outFile.write(reinterpret_cast<char*>(&onseam), sizeof(onseam));
            outFile.write(reinterpret_cast<char*>(&s), sizeof(s));
            outFile.write(reinterpret_cast<char*>(&t), sizeof(t));
        }

        for (const auto& tri : jsonMDL["triangles"]) {
            int facesfront = tri[0].get<bool>() ? 1 : 0;
            int v[3] = { tri[1], tri[2], tri[3] };
            outFile.write(reinterpret_cast<char*>(&facesfront), sizeof(facesfront));
            outFile.write(reinterpret_cast<char*>(v), sizeof(v));
        }

        for (const auto& f : jsonMDL["frames"]) {
            int group = f["group"].get<bool>() ? 1 : 0;
            outFile.write(reinterpret_cast<char*>(&group), sizeof(group));

            auto write_simple_frame = [&](const json& gframe) {
                unsigned char bmin[4] = { gframe["min"][0], gframe["min"][1], gframe["min"][2], gframe["min"][3] };
                unsigned char bmax[4] = { gframe["max"][0], gframe["max"][1], gframe["max"][2], gframe["max"][3] };
                char name[16] = { 0 };
                string sName = gframe["name"];
                strncpy(name, sName.c_str(), 15);

                outFile.write(reinterpret_cast<char*>(bmin), 4);
                outFile.write(reinterpret_cast<char*>(bmax), 4);
                outFile.write(name, 16);

                for (const auto& v : gframe["verts"]) {
                    unsigned char vert[4] = { v[0], v[1], v[2], v[3] };
                    outFile.write(reinterpret_cast<char*>(vert), 4);
                }
                };

            if (group == 0) {
                write_simple_frame(f["frame"][0]);
            }
            else {
                int nb = f["nb"];
                outFile.write(reinterpret_cast<char*>(&nb), sizeof(nb));
                unsigned char gmin[4] = { f["min"][0], f["min"][1], f["min"][2], f["min"][3] };
                unsigned char gmax[4] = { f["max"][0], f["max"][1], f["max"][2], f["max"][3] };
                outFile.write(reinterpret_cast<char*>(gmin), 4);
                outFile.write(reinterpret_cast<char*>(gmax), 4);

                vector<float> times = f["time"].get<vector<float>>();
                outFile.write(reinterpret_cast<char*>(times.data()), times.size() * sizeof(float));

                for (const auto& gframe : f["frames"]) {
                    write_simple_frame(gframe);
                }
            }
        }

        outFile.close();
        cout << "MDL constructed successfully: " << outpath << endl;

    }
    catch (exception& e) {
        cout << "JSON Parsing Error: " << e.what() << endl;
    }
}

Q1_MDL_file ParseQ1MDL(fs::path inpath){
    ifstream inFile(inpath, ios::binary);
    Q1_MDL_file NewMDL;

    inFile.read(reinterpret_cast<char*>(&NewMDL.header.ident), sizeof(NewMDL.header.ident));
    inFile.read(reinterpret_cast<char*>(&NewMDL.header.version), sizeof(NewMDL.header.version));

    inFile.read(reinterpret_cast<char*>(&NewMDL.header.scale), sizeof(NewMDL.header.scale));
    inFile.read(reinterpret_cast<char*>(&NewMDL.header.translate), sizeof(NewMDL.header.translate));
    inFile.read(reinterpret_cast<char*>(&NewMDL.header.boundingRadius), sizeof(NewMDL.header.boundingRadius));
    inFile.read(reinterpret_cast<char*>(&NewMDL.header.eyePosition), sizeof(NewMDL.header.eyePosition));

    inFile.read(reinterpret_cast<char*>(&NewMDL.header.numSkins), sizeof(NewMDL.header.numSkins));
    inFile.read(reinterpret_cast<char*>(&NewMDL.header.skinWidth), sizeof(NewMDL.header.skinWidth));
    inFile.read(reinterpret_cast<char*>(&NewMDL.header.skinHeight), sizeof(NewMDL.header.skinHeight));

    inFile.read(reinterpret_cast<char*>(&NewMDL.header.numVerts), sizeof(NewMDL.header.numVerts));
    inFile.read(reinterpret_cast<char*>(&NewMDL.header.numTris), sizeof(NewMDL.header.numTris));
    inFile.read(reinterpret_cast<char*>(&NewMDL.header.numFrames), sizeof(NewMDL.header.numFrames));

    inFile.read(reinterpret_cast<char*>(&NewMDL.header.syncType), sizeof(NewMDL.header.syncType));
    inFile.read(reinterpret_cast<char*>(&NewMDL.header.flags), sizeof(NewMDL.header.flags));
    inFile.read(reinterpret_cast<char*>(&NewMDL.header.size), sizeof(NewMDL.header.size));

    for (int h = 0; h < NewMDL.header.numSkins; h++) {
        int group;
        inFile.read(reinterpret_cast<char*>(&group), sizeof(group));
        if (group == 0) {
            q1_mdl_skin_t skindata;
            skindata.group = group;
            for (int i = 0; i < NewMDL.header.skinHeight * NewMDL.header.skinWidth; i++) {
                uint8_t texel;
                inFile.read(reinterpret_cast<char*>(&texel), sizeof(texel));
                skindata.data.push_back(texel);
            }
            NewMDL.allskins.push_back(skindata);
        }
        else if (group == 1) {
            q1_mdl_groupskin_t skindata;
            skindata.group = group;
            cout << "Skin group detected." << endl;
            inFile.read(reinterpret_cast<char*>(&skindata.nb), sizeof(skindata.nb));
            for (int i = 0; i < skindata.nb; i++) {
                float duration;
                inFile.read(reinterpret_cast<char*>(&duration), sizeof(duration));
                skindata.time.push_back(duration);
            }
            for (int i = 0; i < skindata.nb; i++) {
                vector<uint8_t> texture;
                for (int j = 0; j < NewMDL.header.skinHeight * NewMDL.header.skinWidth; j++) {
                    uint8_t texel;
                    inFile.read(reinterpret_cast<char*>(&texel), sizeof(texel));
                    texture.push_back(texel);
                }
                skindata.frames.push_back(texture);
            }
            NewMDL.allskins.push_back(skindata);
        }
    }

    for (int i = 0; i < NewMDL.header.numVerts; i++) {
        q1_mdl_texcoord_t vert;
        inFile.read(reinterpret_cast<char*>(&vert.onseam), sizeof(vert.onseam));
        if (vert.onseam != 0 && vert.onseam != 32)
            cout << "Strange UV onseam value detected: " << vert.onseam << endl;
        inFile.read(reinterpret_cast<char*>(&vert.s), sizeof(vert.s));
        inFile.read(reinterpret_cast<char*>(&vert.t), sizeof(vert.t));
        NewMDL.UV.push_back(vert);
    }

    for (int i = 0; i < NewMDL.header.numTris; i++) {
        q1_mdl_triangle_t triangle;
        inFile.read(reinterpret_cast<char*>(&triangle.facesfront), sizeof(triangle.facesfront));
        if (triangle.facesfront != 0 && triangle.facesfront != 1)
            cout << "Strange triangle facesfront value detected: " << triangle.facesfront << endl;
        inFile.read(reinterpret_cast<char*>(&triangle.vertexIndices[0]), sizeof(triangle.vertexIndices[0]));
        inFile.read(reinterpret_cast<char*>(&triangle.vertexIndices[1]), sizeof(triangle.vertexIndices[1]));
        inFile.read(reinterpret_cast<char*>(&triangle.vertexIndices[2]), sizeof(triangle.vertexIndices[2]));
        NewMDL.triangles.push_back(triangle);
    }

    for (int h = 0; h < NewMDL.header.numFrames; h++) {
        int group;
        inFile.read(reinterpret_cast<char*>(&group), sizeof(group));
        if (group == 0) {
            q1_mdl_frame_t newframe;
            newframe.type = group;
            inFile.read(reinterpret_cast<char*>(&newframe.frame.bboxmin), sizeof(newframe.frame.bboxmin));
            inFile.read(reinterpret_cast<char*>(&newframe.frame.bboxmax), sizeof(newframe.frame.bboxmax));
            inFile.read(newframe.frame.name, 16);
            for (int i = 0; i < NewMDL.header.numVerts; i++) {
                q1_mdl_vertex_t vert;
                inFile.read(reinterpret_cast<char*>(&vert.v[0]), sizeof(vert.v[0]));
                inFile.read(reinterpret_cast<char*>(&vert.v[1]), sizeof(vert.v[1]));
                inFile.read(reinterpret_cast<char*>(&vert.v[2]), sizeof(vert.v[2]));
                inFile.read(reinterpret_cast<char*>(&vert.normalIndex), sizeof(vert.normalIndex));
                newframe.frame.verts.push_back(vert);
            }
            NewMDL.allframes.push_back(newframe);
        }
        else if (group == 1) {
            cout << "Frame group detected." << endl;
            q1_mdl_groupframe_t newframegroup;
            newframegroup.type = group;
            inFile.read(reinterpret_cast<char*>(&newframegroup.nb), sizeof(newframegroup.nb));

            inFile.read(reinterpret_cast<char*>(&newframegroup.min.v[0]), sizeof(newframegroup.min.v[0]));
            inFile.read(reinterpret_cast<char*>(&newframegroup.min.v[1]), sizeof(newframegroup.min.v[1]));
            inFile.read(reinterpret_cast<char*>(&newframegroup.min.v[2]), sizeof(newframegroup.min.v[2]));
            inFile.read(reinterpret_cast<char*>(&newframegroup.min.normalIndex), sizeof(newframegroup.min.normalIndex));

            inFile.read(reinterpret_cast<char*>(&newframegroup.max.v[0]), sizeof(newframegroup.max.v[0]));
            inFile.read(reinterpret_cast<char*>(&newframegroup.max.v[1]), sizeof(newframegroup.max.v[1]));
            inFile.read(reinterpret_cast<char*>(&newframegroup.max.v[2]), sizeof(newframegroup.max.v[2]));
            inFile.read(reinterpret_cast<char*>(&newframegroup.max.normalIndex), sizeof(newframegroup.max.normalIndex));

            for (int i = 0; i < newframegroup.nb; i++) {
                float duration;
                inFile.read(reinterpret_cast<char*>(&duration), sizeof(duration));
                newframegroup.time.push_back(duration);
            }

            for (int i = 0; i < newframegroup.nb; i++) {
                q1_mdl_simpleframe_t newframeentry;
                inFile.read(reinterpret_cast<char*>(&newframeentry.bboxmin), sizeof(newframeentry.bboxmin));
                inFile.read(reinterpret_cast<char*>(&newframeentry.bboxmax), sizeof(newframeentry.bboxmax));
                inFile.read(newframeentry.name, 16);
                for (int i = 0; i < NewMDL.header.numVerts; i++) {
                    q1_mdl_vertex_t vert;
                    inFile.read(reinterpret_cast<char*>(&vert.v[0]), sizeof(vert.v[0]));
                    inFile.read(reinterpret_cast<char*>(&vert.v[1]), sizeof(vert.v[1]));
                    inFile.read(reinterpret_cast<char*>(&vert.v[2]), sizeof(vert.v[2]));
                    inFile.read(reinterpret_cast<char*>(&vert.normalIndex), sizeof(vert.normalIndex));
                    newframeentry.verts.push_back(vert);
                }
                newframegroup.frames.push_back(newframeentry);
            }
            NewMDL.allframes.push_back(newframegroup);
        }
    }
    inFile.close();

    return NewMDL;
    }

void Q1MDL2JSON(const Q1_MDL_file& NewMDL, fs::path outpath) {
    
    json jsonMDL;
    json jheader;
    jheader["ident"] = "IDPO";
    jheader["version"] = NewMDL.header.version;
    jheader["scale"] = { NewMDL.header.scale[0], NewMDL.header.scale[1], NewMDL.header.scale[2] };
    jheader["translate"] = { NewMDL.header.translate[0], NewMDL.header.translate[1], NewMDL.header.translate[2] };
    jheader["boundingRadius"] = NewMDL.header.boundingRadius;
    jheader["eyePosition"] = { NewMDL.header.eyePosition[0], NewMDL.header.eyePosition[1], NewMDL.header.eyePosition[2] };
    jheader["numSkins"] = NewMDL.header.numSkins;
    jheader["skinWidth"] = NewMDL.header.skinWidth;
    jheader["skinHeight"] = NewMDL.header.skinHeight;
    jheader["numVerts"] = NewMDL.header.numVerts;
    jheader["numTris"] = NewMDL.header.numTris;
    jheader["numFrames"] = NewMDL.header.numFrames;
    jheader["syncType"] = NewMDL.header.syncType;
    jheader["flags"] = NewMDL.header.flags;
    jheader["size"] = NewMDL.header.size;
    jsonMDL["header"] = jheader;

    jsonMDL["skins"] = json::array();
    for (const auto& skin : NewMDL.allskins) {
        visit([&](auto&& arg) {
            json s;
            s["group"] = (bool)arg.group;
            using T = decay_t<decltype(arg)>;
            if constexpr (is_same_v<T, q1_mdl_groupskin_t>) {
                s["nb"] = arg.nb;
                s["time"] = arg.time;
                s["skins"] = arg.frames;
            }
            else {
                s["skin"] = arg.data;
            }
            jsonMDL["skins"].push_back(s);
            }, skin);
    }

    jsonMDL["UV"] = json::array();
    for (const auto& vert : NewMDL.UV) {
        json s;
        s = { (vert.onseam != 0),vert.s,vert.t };
        jsonMDL["UV"].push_back(s);
    }
    jsonMDL["triangles"] = json::array();;
    for (const auto& tri : NewMDL.triangles) {
        json s;
        s = { (bool)tri.facesfront,tri.vertexIndices[0],tri.vertexIndices[1],tri.vertexIndices[2] };
        jsonMDL["triangles"].push_back(s);
    }

    jsonMDL["frames"] = json::array();
    for (const auto& frame : NewMDL.allframes) {
        visit([&](auto&& arg) {
            json s;
            s["group"] = (bool)arg.type;
            using T = decay_t<decltype(arg)>;
            if constexpr (is_same_v<T, q1_mdl_groupframe_t>) {
                s["nb"] = arg.nb;
                s["min"] = { arg.min.v[0],arg.min.v[1],arg.min.v[2],arg.min.normalIndex };
                s["max"] = { arg.max.v[0],arg.max.v[1],arg.max.v[2],arg.max.normalIndex };
                s["time"] = arg.time;

                s["frames"] = json::array();
                for (int j = 0; j < arg.nb; j++) {
                    json gframe;
                    gframe["min"] = { arg.frames[j].bboxmin.v[0],arg.frames[j].bboxmin.v[1],arg.frames[j].bboxmin.v[2],arg.frames[j].bboxmin.normalIndex };
                    gframe["max"] = { arg.frames[j].bboxmax.v[0],arg.frames[j].bboxmax.v[1],arg.frames[j].bboxmax.v[2],arg.frames[j].bboxmax.normalIndex };
                    gframe["name"] = arg.frames[j].name;
                    gframe["verts"] = json::array();
                    for (int k = 0; k < NewMDL.header.numVerts; k++) {
                        json verts;
                        verts = { arg.frames[j].verts[k].v[0],arg.frames[j].verts[k].v[1],arg.frames[j].verts[k].v[2],arg.frames[j].verts[k].normalIndex };
                        gframe["verts"].push_back(verts);
                    }
                    s["frames"].push_back(gframe);
                }
            }
            else {
                json gframe;
                gframe["min"] = { arg.frame.bboxmin.v[0],arg.frame.bboxmin.v[1],arg.frame.bboxmin.v[2],arg.frame.bboxmin.normalIndex };
                gframe["max"] = { arg.frame.bboxmax.v[0],arg.frame.bboxmax.v[1],arg.frame.bboxmax.v[2],arg.frame.bboxmax.normalIndex };
                gframe["name"] = arg.frame.name;
                gframe["verts"] = json::array();
                for (int i = 0; i < NewMDL.header.numVerts; i++) {
                    json verts;
                    verts = { arg.frame.verts[i].v[0],arg.frame.verts[i].v[1],arg.frame.verts[i].v[2],arg.frame.verts[i].normalIndex };
                    gframe["verts"].push_back(verts);
                }
                s["frame"].push_back(gframe);
            }
            jsonMDL["frames"].push_back(s);
            }, frame);
    }

    ofstream outFile(outpath);
    if (outFile.is_open()) {
        outFile << jsonMDL.dump(2);
        outFile.close();
        cout << "JSON created successfully: " << outpath << endl;
    }
    else {
        cout << "Error: Could not write output file." << endl;
    }
}