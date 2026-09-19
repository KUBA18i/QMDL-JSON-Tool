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

#include "H2PoPMDL.h"

using namespace std;
using json = nlohmann::ordered_json;
namespace fs = filesystem;


H2PoP_MDL_file JSON2H2PoPMDL(fs::path inpath, json jsonMDL) {
    try {
        H2PoP_MDL_file newMDL;
        newMDL.header.numSkins = jsonMDL["skins"].size();
        newMDL.header.numTris = jsonMDL["triangles"].size();
        newMDL.header.numFrames = jsonMDL["frames"].size();
        newMDL.header.numStVerts = jsonMDL["UV"].size(); //RAPO addition

        newMDL.header.numVerts = -1;
        for (const auto& f : jsonMDL["frames"]) {
            if (f["group"].get<bool>() == false) {
                auto& sframe = f["frame"][0];
                if (newMDL.header.numVerts == -1)
                    newMDL.header.numVerts = sframe["verts"].size();
                else if (newMDL.header.numVerts != sframe["verts"].size()) {
                    cout << "Error: Frame " << (f["frame"][0]["name"]) << " has " << sframe["verts"].size() << " verts, but expected " << newMDL.header.numVerts << endl;
                    exit(1);
                }
            }
            else {
                for (const auto& gframe : f["frames"]) {
                    if (newMDL.header.numVerts == -1)
                        newMDL.header.numVerts = gframe["verts"].size();
                    else if (gframe["verts"].size() != newMDL.header.numVerts) {
                        cout << "Error: Subframe " << gframe["name"] << " in group has " << gframe["verts"].size() << " verts, but expected " << newMDL.header.numVerts << endl;
                        exit(1);
                    }
                }
            }
        }

        newMDL.header.ident = 0x4F504152;//RAPO
        newMDL.header.version = 50;

        auto jheader = jsonMDL.at("header");
        newMDL.header.scale[0] = jheader["scale"][0];
        newMDL.header.scale[1] = jheader["scale"][1];
        newMDL.header.scale[2] = jheader["scale"][2];
        newMDL.header.translate[0] = jheader["translate"][0];
        newMDL.header.translate[1] = jheader["translate"][1];
        newMDL.header.translate[2] = jheader["translate"][2];
        newMDL.header.boundingRadius = jheader["boundingRadius"];
        newMDL.header.eyePosition[0] = jheader["eyePosition"][0];
        newMDL.header.eyePosition[1] = jheader["eyePosition"][1];
        newMDL.header.eyePosition[2] = jheader["eyePosition"][2];
        newMDL.header.skinWidth = jheader["skinWidth"];
        newMDL.header.skinHeight = jheader["skinHeight"];
        newMDL.header.syncType = jheader["syncType"];
        newMDL.header.flags = jheader["flags"];
        newMDL.header.size = jheader["size"];

        for (const auto& s : jsonMDL["skins"]) {
            int group = s["group"].get<bool>() ? 1 : 0;
            if (group == 0) {
                h2pop_mdl_skin_t newSkin;
                newSkin.group = 0;
                newSkin.data = s["skin"].get<vector<uint8_t>>();
                newMDL.allskins.push_back(newSkin);
            }
            else {
                h2pop_mdl_groupskin_t newSkinGroup;
                newSkinGroup.group = 1;
                newSkinGroup.nb = s["nb"];
                newSkinGroup.time = s["time"].get<vector<float>>();
                for (const auto& frameData : s["skins"]) {
                    vector<uint8_t> data = frameData.get<vector<uint8_t>>();
                    newSkinGroup.frames.push_back(data);
                }
                newMDL.allskins.push_back(newSkinGroup);
            }
        }

        for (const auto& uv : jsonMDL["UV"]) {
            h2pop_mdl_texcoord_t newTC;
            newTC.onseam = uv[0].get<bool>() ? 32 : 0;
            newTC.s = uv[1];
            newTC.t = uv[2];
            newMDL.UV.push_back(newTC);
        }

        for (const auto& tri : jsonMDL["triangles"]) {
            h2pop_mdl_triangle_t newTriangle;
            newTriangle.facesfront = tri["facesfront"].get<bool>() ? 1 : 0;
            newTriangle.vertindex[0] = tri["vertindices"][0];
            newTriangle.vertindex[1] = tri["vertindices"][1];
            newTriangle.vertindex[2] = tri["vertindices"][2];
            newTriangle.stindex[0] = tri["stindices"][0];
            newTriangle.stindex[1] = tri["stindices"][1];
            newTriangle.stindex[2] = tri["stindices"][2];
            newMDL.triangles.push_back(newTriangle);
        }

        for (const auto& f : jsonMDL["frames"]) {
            int group = f["group"].get<bool>() ? 1 : 0;
            auto make_simple_frame = [&](const json& gframe) {
                h2pop_mdl_simpleframe_t newSimpleFrame;
                newSimpleFrame.bboxmin.v[0] = gframe["min"][0];
                newSimpleFrame.bboxmin.v[1] = gframe["min"][1];
                newSimpleFrame.bboxmin.v[2] = gframe["min"][2];
                newSimpleFrame.bboxmin.normalIndex = gframe["min"][3];
                newSimpleFrame.bboxmax.v[0] = gframe["max"][0];
                newSimpleFrame.bboxmax.v[1] = gframe["max"][1];
                newSimpleFrame.bboxmax.v[2] = gframe["max"][2];
                newSimpleFrame.bboxmax.normalIndex = gframe["max"][3];
                string sName = gframe["name"];
                memset(newSimpleFrame.name, 0, sizeof(newSimpleFrame.name));
                strncpy(newSimpleFrame.name, sName.c_str(), sizeof(newSimpleFrame.name) - 1);

                for (const auto& v : gframe["verts"]) {
                    h2pop_mdl_vertex_t newVert;
                    newVert.v[0] = v[0];
                    newVert.v[1] = v[1];
                    newVert.v[2] = v[2];
                    newVert.normalIndex = v[3];
                    newSimpleFrame.verts.push_back(newVert);
                }

                return newSimpleFrame;
                };

            if (group == 0) {
                h2pop_mdl_frame_t NewFrame;
                NewFrame.type = 0;
                NewFrame.frame = make_simple_frame(f["frame"][0]);
                newMDL.allframes.push_back(NewFrame);
            }
            else {
                h2pop_mdl_groupframe_t NewFrameGroup;
                NewFrameGroup.type = 1;
                NewFrameGroup.nb = f["nb"];
                NewFrameGroup.min.v[0] = f["min"][0];
                NewFrameGroup.min.v[1] = f["min"][1];
                NewFrameGroup.min.v[2] = f["min"][2];
                NewFrameGroup.min.normalIndex = f["min"][3];
                NewFrameGroup.max.v[0] = f["max"][0];
                NewFrameGroup.max.v[1] = f["max"][1];
                NewFrameGroup.max.v[2] = f["max"][2];
                NewFrameGroup.max.normalIndex = f["max"][3];
                NewFrameGroup.time = f["time"].get<vector<float>>();
                for (const auto& gframe : f["frames"])
                    NewFrameGroup.frames.push_back(make_simple_frame(gframe));
                newMDL.allframes.push_back(NewFrameGroup);
            }
        }
        cout << "JSON parsed, creating MDL..." << endl;
        return newMDL;
    }
    catch (exception& e) {
        cout << "JSON Parsing Error: " << e.what() << endl;
    }
}

void WriteH2PoPMDL(fs::path outpath, const H2PoP_MDL_file newMDL) {
    try {
        ofstream outFile(outpath, ios::binary);
        if (!outFile.is_open()) {
            cout << "Error: Could not open output file." << endl;
            return;
        }

        outFile.write("RAPO", 4);
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.version), sizeof(newMDL.header.version));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.scale), sizeof(newMDL.header.scale));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.translate), sizeof(newMDL.header.translate));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.boundingRadius), sizeof(newMDL.header.boundingRadius));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.eyePosition), sizeof(newMDL.header.eyePosition));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.numSkins), sizeof(newMDL.header.numSkins));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.skinWidth), sizeof(newMDL.header.skinWidth));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.skinHeight), sizeof(newMDL.header.skinHeight));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.numVerts), sizeof(newMDL.header.numVerts));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.numTris), sizeof(newMDL.header.numTris));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.numFrames), sizeof(newMDL.header.numFrames));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.syncType), sizeof(newMDL.header.syncType));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.flags), sizeof(newMDL.header.flags));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.size), sizeof(newMDL.header.size));
        outFile.write(reinterpret_cast<const char*>(&newMDL.header.numStVerts), sizeof(newMDL.header.numStVerts)); //RAPO addition

        for (const auto& skin : newMDL.allskins) {
            visit([&](auto&& arg) {
                outFile.write(reinterpret_cast<const char*>(&arg.group), sizeof(arg.group));
                using T = decay_t<decltype(arg)>;
                if constexpr (is_same_v<T, h2pop_mdl_groupskin_t>) {
                    outFile.write(reinterpret_cast<const char*>(&arg.nb), sizeof(arg.nb));
                    outFile.write(reinterpret_cast<const char*>(arg.time.data()), arg.time.size() * sizeof(float));
                    for (const auto& frameData : arg.frames) {
                        outFile.write(reinterpret_cast<const char*>(frameData.data()), frameData.size());
                    }
                }
                else
                    outFile.write(reinterpret_cast<const char*>(arg.data.data()), arg.data.size());
                }, skin);
        }

        for (const auto& uv : newMDL.UV) {
            outFile.write(reinterpret_cast<const char*>(&uv.onseam), sizeof(uv.onseam));
            outFile.write(reinterpret_cast<const char*>(&uv.s), sizeof(uv.s));
            outFile.write(reinterpret_cast<const char*>(&uv.t), sizeof(uv.t));
        }

        for (const auto& tri : newMDL.triangles) {
            outFile.write(reinterpret_cast<const char*>(&tri.facesfront), sizeof(tri.facesfront));
            outFile.write(reinterpret_cast<const char*>(&tri.vertindex), sizeof(tri.vertindex));
            outFile.write(reinterpret_cast<const char*>(&tri.stindex), sizeof(tri.stindex));
        }

        for (const auto& frame : newMDL.allframes) {
            auto write_simple_frame = [&](const h2pop_mdl_simpleframe_t sframe) {
                outFile.write(reinterpret_cast<const char*>(&sframe.bboxmin), sizeof(sframe.bboxmin));
                outFile.write(reinterpret_cast<const char*>(&sframe.bboxmax), sizeof(sframe.bboxmax));
                outFile.write(sframe.name, 16);
                for (const auto& v : sframe.verts)
                    outFile.write(reinterpret_cast<const char*>(&v), sizeof(v));
                };
            visit([&](auto&& arg) {
                outFile.write(reinterpret_cast<const char*>(&arg.type), sizeof(arg.type));
                using T = decay_t<decltype(arg)>;
                if constexpr (is_same_v<T, h2pop_mdl_groupframe_t>) {
                    outFile.write(reinterpret_cast<const char*>(&arg.nb), sizeof(arg.nb));
                    outFile.write(reinterpret_cast<const char*>(&arg.min), sizeof(arg.min));
                    outFile.write(reinterpret_cast<const char*>(&arg.max), sizeof(arg.max));
                    outFile.write(reinterpret_cast<const char*>(arg.time.data()), arg.time.size() * sizeof(float));
                    for (const auto& sf : arg.frames)
                        write_simple_frame(sf);
                }
                else
                    write_simple_frame(arg.frame);
                }, frame);
        }

        outFile.close();
        cout << "MDL constructed successfully: " << outpath << endl;

    }
    catch (exception& e) {
        cout << "Error writing file: " << e.what() << endl;
    }
}

H2PoP_MDL_file ReadH2PoPMDL(fs::path inpath) {
    ifstream inFile(inpath, ios::binary);
    H2PoP_MDL_file NewMDL;

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

    inFile.read(reinterpret_cast<char*>(&NewMDL.header.numStVerts), sizeof(NewMDL.header.numStVerts)); //RAPO addition

    for (int h = 0; h < NewMDL.header.numSkins; h++) {
        int group;
        inFile.read(reinterpret_cast<char*>(&group), sizeof(group));
        if (group == 0) {
            h2pop_mdl_skin_t skindata;
            skindata.group = group;
            for (int i = 0; i < NewMDL.header.skinHeight * NewMDL.header.skinWidth; i++) {
                uint8_t texel;
                inFile.read(reinterpret_cast<char*>(&texel), sizeof(texel));
                skindata.data.push_back(texel);
            }
            NewMDL.allskins.push_back(skindata);
        }
        else if (group == 1) {
            h2pop_mdl_groupskin_t skindata;
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

    //In RAPO, you don't use header.num_verts here
    for (int i = 0; i < NewMDL.header.numStVerts; i++) {
        h2pop_mdl_texcoord_t vert;
        inFile.read(reinterpret_cast<char*>(&vert.onseam), sizeof(vert.onseam));
        if (vert.onseam != 0 && vert.onseam != 32)
            cout << "Strange UV onseam value detected: " << vert.onseam << endl;
        inFile.read(reinterpret_cast<char*>(&vert.s), sizeof(vert.s));
        inFile.read(reinterpret_cast<char*>(&vert.t), sizeof(vert.t));
        NewMDL.UV.push_back(vert);
    }

    for (int i = 0; i < NewMDL.header.numTris; i++) {
        h2pop_mdl_triangle_t triangle;
        inFile.read(reinterpret_cast<char*>(&triangle.facesfront), sizeof(triangle.facesfront));
        if (triangle.facesfront != 0 && triangle.facesfront != 1)
            cout << "Strange triangle facesfront value detected: " << triangle.facesfront << endl;

        inFile.read(reinterpret_cast<char*>(&triangle.vertindex[0]), sizeof(triangle.vertindex[0]));
        inFile.read(reinterpret_cast<char*>(&triangle.vertindex[1]), sizeof(triangle.vertindex[1]));
        inFile.read(reinterpret_cast<char*>(&triangle.vertindex[2]), sizeof(triangle.vertindex[2]));

        inFile.read(reinterpret_cast<char*>(&triangle.stindex[0]), sizeof(triangle.stindex[0]));
        inFile.read(reinterpret_cast<char*>(&triangle.stindex[1]), sizeof(triangle.stindex[1]));
        inFile.read(reinterpret_cast<char*>(&triangle.stindex[2]), sizeof(triangle.stindex[2]));

        NewMDL.triangles.push_back(triangle);
    }

    for (int h = 0; h < NewMDL.header.numFrames; h++) {
        int group;
        inFile.read(reinterpret_cast<char*>(&group), sizeof(group));
        if (group == 0) {
            h2pop_mdl_frame_t newframe;
            newframe.type = group;
            inFile.read(reinterpret_cast<char*>(&newframe.frame.bboxmin), sizeof(newframe.frame.bboxmin));
            inFile.read(reinterpret_cast<char*>(&newframe.frame.bboxmax), sizeof(newframe.frame.bboxmax));
            inFile.read(newframe.frame.name, 16);
            for (int i = 0; i < NewMDL.header.numVerts; i++) {
                h2pop_mdl_vertex_t vert;
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
            h2pop_mdl_groupframe_t newframegroup;
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
                h2pop_mdl_simpleframe_t newframeentry;
                inFile.read(reinterpret_cast<char*>(&newframeentry.bboxmin), sizeof(newframeentry.bboxmin));
                inFile.read(reinterpret_cast<char*>(&newframeentry.bboxmax), sizeof(newframeentry.bboxmax));
                inFile.read(newframeentry.name, 16);
                for (int i = 0; i < NewMDL.header.numVerts; i++) {
                    h2pop_mdl_vertex_t vert;
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

void H2PoPMDL2JSON(const H2PoP_MDL_file& NewMDL, fs::path outpath) {

    json jsonMDL;
    json jheader;
    jheader["ident"] = "RAPO";
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
    jheader["numStVerts"] = NewMDL.header.numStVerts;
    jsonMDL["header"] = jheader;

    jsonMDL["skins"] = json::array();
    for (const auto& skin : NewMDL.allskins) {
        visit([&](auto&& arg) {
            json s;
            s["group"] = (bool)arg.group;
            using T = decay_t<decltype(arg)>;
            if constexpr (is_same_v<T, h2pop_mdl_groupskin_t>) {
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
        s["facesfront"] = (bool)tri.facesfront;
        s["vertindices"] = { tri.vertindex[0],tri.vertindex[1],tri.vertindex[2] };
        s["stindices"] = { tri.stindex[0],tri.stindex[1],tri.stindex[2] };
        jsonMDL["triangles"].push_back(s);
    }

    jsonMDL["frames"] = json::array();
    for (const auto& frame : NewMDL.allframes) {
        visit([&](auto&& arg) {
            json s;
            s["group"] = (bool)arg.type;
            using T = decay_t<decltype(arg)>;
            if constexpr (is_same_v<T, h2pop_mdl_groupframe_t>) {
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