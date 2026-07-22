#include "H2FM.h"

void WriteBlockHeader(ofstream& outFile, string_view ident, int version, int size) {
    char identBuffer[32] = { 0 };
    size_t copyLen = min(ident.size(), sizeof(identBuffer) - 1);
    memcpy(identBuffer, ident.data(), copyLen);
    outFile.write(identBuffer, sizeof(identBuffer));
    outFile.write(reinterpret_cast<const char*>(&version), sizeof(int));
    outFile.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
}

void JSON2H2FM(fs::path inpath, fs::path outpath, json jsonFM) {
    try {
        ofstream outFile(outpath, ios::binary);
        if (!outFile.is_open()) {
            cout << "Error: Could not open output file." << endl;
            return;
        }

        WriteBlockHeader(outFile, FM_HEADER_NAME, FM_HEADER_VER, sizeof(h2_fm_startheader_t));
        h2_fm_startheader_t new_header;

        new_header.num_skins = jsonFM["skins"].size();
        new_header.num_st = jsonFM["UV"].size();
        new_header.num_tris = jsonFM["triangles"].size();
        new_header.num_frames = jsonFM["frames"].size();
        new_header.num_mesh_nodes = jsonFM["meshNodes"].size();

        new_header.num_xyz = -1;
        for (const auto& f : jsonFM["frames"]) {
            if (new_header.num_xyz == -1)
                new_header.num_xyz = f["verts"].size();
            else if (new_header.num_xyz != f["verts"].size()) {
                cout << "Error: Frame " << (f["name"]) << " has " << f["verts"].size() << " verts, but expected " << new_header.num_xyz << endl;
                return;
            }
        }

        new_header.num_glcmds = 0;
        for (const auto& g : jsonFM["glCommands"]) {
            new_header.num_glcmds++;
            new_header.num_glcmds += (g["verts"].size() * 3);
        }
        new_header.num_glcmds += new_header.num_mesh_nodes;

        new_header.framesize = new_header.num_xyz * 4 + 40;

        auto jheader = jsonFM.at("header");
        new_header.skinwidth = jheader["skinwidth"];
        new_header.skinheight = jheader["skinheight"];

        outFile.write(reinterpret_cast<char*>(&new_header.skinwidth), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.skinheight), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.framesize), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.num_skins), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.num_xyz), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.num_st), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.num_tris), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.num_glcmds), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.num_frames), sizeof(int));
        outFile.write(reinterpret_cast<char*>(&new_header.num_mesh_nodes), sizeof(int));

        WriteBlockHeader(outFile, FM_SKIN_NAME, FM_SKIN_VER, new_header.num_skins * 64);
        for (const auto& s : jsonFM["skins"]) {
            char name[64] = { 0 };
            string sName = s;
            strncpy(name, sName.c_str(), 63);
            outFile.write(name, 64);
        }

        WriteBlockHeader(outFile, FM_ST_NAME, FM_ST_VER, new_header.num_st * 4);
        for (const auto& uv : jsonFM["UV"]) {
            short s = uv[0];
            short t = uv[1];
            outFile.write(reinterpret_cast<char*>(&s), sizeof(s));
            outFile.write(reinterpret_cast<char*>(&t), sizeof(t));
        }

        WriteBlockHeader(outFile, FM_TRI_NAME, FM_TRI_VER, new_header.num_tris * 12);
        for (const auto& tri : jsonFM["triangles"]) {
            short t[6] = { tri[0], tri[1], tri[2], tri[3], tri[4], tri[5] };
            outFile.write(reinterpret_cast<char*>(t), sizeof(t));
        }

        WriteBlockHeader(outFile, FM_FRAME_NAME, FM_FRAME_VER, new_header.framesize * new_header.num_frames);
        for (const auto& f : jsonFM["frames"]) {
            float scale[3] = { f["scale"][0], f["scale"][1], f["scale"][2] };
            float translate[3] = { f["translate"][0], f["translate"][1], f["translate"][2] };
            char name[16] = { 0 };
            string sName = f["name"];
            strncpy(name, sName.c_str(), 15);

            outFile.write(reinterpret_cast<char*>(&scale), sizeof(scale));
            outFile.write(reinterpret_cast<char*>(&translate), sizeof(translate));
            outFile.write(name, 16);

            for (const auto& v : f["verts"]) {
                uint8_t vert[4] = { v[0], v[1], v[2], v[3] };
                outFile.write(reinterpret_cast<char*>(&vert), 4);
            }
        }

        int zeroCmd = 0;
        WriteBlockHeader(outFile, FM_GLCMDS_NAME, FM_GLCMDS_VER, new_header.num_glcmds * 4);
        int current_glcmd_idx = 0;
        size_t current_mesh_node = 0;
        for (const auto& g : jsonFM["glCommands"]) {
            if (current_mesh_node + 1 < jsonFM["meshNodes"].size()) {
                int next_start = jsonFM["meshNodes"][current_mesh_node + 1]["start_glcmds"].get<int>();
                if (current_glcmd_idx == next_start - 1) {
                    outFile.write(reinterpret_cast<char*>(&zeroCmd), sizeof(int));
                    current_glcmd_idx++;
                    current_mesh_node++;
                }
            }
            int vcount = g["verts"].size();
            if (g["strip"].get<bool>() == false) vcount *= (-1);
            outFile.write(reinterpret_cast<char*>(&vcount), sizeof(vcount));
            current_glcmd_idx++;
            for (const auto& v : g["verts"]) {
                float vs = v[0];
                float vt = v[1];
                int vertexIndex = v[2];
                outFile.write(reinterpret_cast<char*>(&vs), sizeof(vs));
                outFile.write(reinterpret_cast<char*>(&vt), sizeof(vt));
                outFile.write(reinterpret_cast<char*>(&vertexIndex), sizeof(vertexIndex));
                current_glcmd_idx += 3;
            }
        }
        outFile.write(reinterpret_cast<char*>(&zeroCmd), sizeof(int));
        current_glcmd_idx++;
        
        WriteBlockHeader(outFile, FM_MESH_NAME, FM_MESH_VER, new_header.num_mesh_nodes * sizeof(h2_fm_meshnode_t));
        for (const auto& mn : jsonFM["meshNodes"]) {
            for (const auto& u : mn["unused"]) {
                uint8_t newun = u;
                outFile.write(reinterpret_cast<char*>(&newun), sizeof(newun));
            }
            for (const auto& v : mn["verts"]) {
                uint8_t newvert = v;
                outFile.write(reinterpret_cast<char*>(&newvert), sizeof(newvert));
            }
            short start = mn["start_glcmds"];
            outFile.write(reinterpret_cast<char*>(&start), sizeof(start));
            short end = mn["num_glcmds"];
            outFile.write(reinterpret_cast<char*>(&end), sizeof(end));
        }
        
        if (jsonFM.contains("skeleton") && !jsonFM["skeleton"].is_null()) {
            cout << "Skeleton data block present." << endl;
            h2_fm_SkeletonBlock_t newskelblock;
            int skelblocksize = sizeof(int) * 3;
            auto jskeleton = jsonFM.at("skeleton");
            newskelblock.skeletalType = jskeleton["skeletalType"];
            newskelblock.numClusters = jskeleton["clusters"].size();
            skelblocksize += newskelblock.numClusters * sizeof(int);
            for (const auto& c : jskeleton["clusters"]) {
                h2_fm_SkeletalCluster_t newc;
                for (const auto& v : c)
                    newc.vertices.push_back(v.get<int>());
                newskelblock.clusters.push_back(newc);
                skelblocksize += newc.vertices.size() * sizeof(int);
            }
            newskelblock.haveSkeleton = jskeleton["haveSkeleton"].get<bool>() ? 1 : 0;
            if (newskelblock.haveSkeleton) {
                for (const auto& f : jskeleton["frames"]) {
                    h2_fm_SkeletonFrame_t newframe;
                    for (const auto& p : f) {
                        h2_fm_Placement_t newp;
                        newp.origin[0] = p[0];
                        newp.origin[1] = p[1];
                        newp.origin[2] = p[2];
                        newp.direction[0] = p[3];
                        newp.direction[1] = p[4];
                        newp.direction[2] = p[5];
                        newp.up[0] = p[6];
                        newp.up[1] = p[7];
                        newp.up[2] = p[8];
                        newframe.joints.push_back(newp);
                    }
                    newskelblock.frames.push_back(newframe);
                    skelblocksize += newframe.joints.size() * sizeof(h2_fm_Placement_t);
                }
            }
            
            WriteBlockHeader(outFile, FM_SKELETON_NAME, FM_SKELETON_VER, skelblocksize);
            outFile.write(reinterpret_cast<char*>(&newskelblock.skeletalType), sizeof(newskelblock.skeletalType));
            outFile.write(reinterpret_cast<char*>(&newskelblock.numClusters), sizeof(newskelblock.numClusters));
            for (const auto& c : newskelblock.clusters) {
                int count = c.vertices.size();
                outFile.write(reinterpret_cast<char*>(&count), sizeof(int));
            }
            for (const auto& c : newskelblock.clusters)
                for (auto v : c.vertices)
                    outFile.write(reinterpret_cast<char*>(&v), sizeof(v));
            outFile.write(reinterpret_cast<char*>(&newskelblock.haveSkeleton), sizeof(newskelblock.haveSkeleton));
            if (newskelblock.haveSkeleton)
                for (const auto& f : newskelblock.frames)
                    for (auto j : f.joints) {
                        outFile.write(reinterpret_cast<char*>(&j.origin[0]), sizeof(j.origin[0]));
                        outFile.write(reinterpret_cast<char*>(&j.origin[1]), sizeof(j.origin[1]));
                        outFile.write(reinterpret_cast<char*>(&j.origin[2]), sizeof(j.origin[2]));
                        outFile.write(reinterpret_cast<char*>(&j.direction[0]), sizeof(j.direction[0]));
                        outFile.write(reinterpret_cast<char*>(&j.direction[1]), sizeof(j.direction[1]));
                        outFile.write(reinterpret_cast<char*>(&j.direction[2]), sizeof(j.direction[2]));
                        outFile.write(reinterpret_cast<char*>(&j.up[0]), sizeof(j.up[0]));
                        outFile.write(reinterpret_cast<char*>(&j.up[1]), sizeof(j.up[1]));
                        outFile.write(reinterpret_cast<char*>(&j.up[2]), sizeof(j.up[2]));
                    }  
        }
        else
            cout << "Skeleton data is absent, skipping." << endl;

        if (jsonFM.contains("references") && !jsonFM["references"].is_null()) {
            cout << "Reference data block present." << endl;
            h2_fm_ReferenceBlock_t newrefblock;
            auto jref = jsonFM.at("references");
            int refblocksize = (sizeof(int) * 2) + (jref["placements"].size() * sizeof(h2_fm_Placement_t));
            newrefblock.referenceType = jref["referenceType"];
            newrefblock.haveRefs = jref["haveRefs"].get<bool>() ? 1 : 0;
            if (newrefblock.haveRefs)
                for (const auto& p : jref["placements"]) {
                    h2_fm_Placement_t newp;
                    newp.origin[0] = p[0];
                    newp.origin[1] = p[1];
                    newp.origin[2] = p[2];
                    newp.direction[0] = p[3];
                    newp.direction[1] = p[4];
                    newp.direction[2] = p[5];
                    newp.up[0] = p[6];
                    newp.up[1] = p[7];
                    newp.up[2] = p[8];
                    newrefblock.placements.push_back(newp);
                }

            WriteBlockHeader(outFile, FM_REFERENCES_NAME, FM_REFERENCES_VER, refblocksize);
            outFile.write(reinterpret_cast<char*>(&newrefblock.referenceType), sizeof(newrefblock.referenceType));
            outFile.write(reinterpret_cast<char*>(&newrefblock.haveRefs), sizeof(newrefblock.haveRefs));
            if (newrefblock.haveRefs)
                for (auto p : newrefblock.placements) {
                    outFile.write(reinterpret_cast<char*>(&p.origin[0]), sizeof(p.origin[0]));
                    outFile.write(reinterpret_cast<char*>(&p.origin[1]), sizeof(p.origin[1]));
                    outFile.write(reinterpret_cast<char*>(&p.origin[2]), sizeof(p.origin[2]));
                    outFile.write(reinterpret_cast<char*>(&p.direction[0]), sizeof(p.direction[0]));
                    outFile.write(reinterpret_cast<char*>(&p.direction[1]), sizeof(p.direction[1]));
                    outFile.write(reinterpret_cast<char*>(&p.direction[2]), sizeof(p.direction[2]));
                    outFile.write(reinterpret_cast<char*>(&p.up[0]), sizeof(p.up[0]));
                    outFile.write(reinterpret_cast<char*>(&p.up[1]), sizeof(p.up[1]));
                    outFile.write(reinterpret_cast<char*>(&p.up[2]), sizeof(p.up[2]));
                }
        }
        else
            cout << "Reference data is absent, skipping." << endl;

        outFile.close();
        cout << "FM constructed successfully: " << outpath << endl;

    }
    catch (exception& e) {
        cout << "JSON Parsing Error: " << e.what() << endl;
    }
}


h2_fm_model_t ParseFM(fs::path filePath) {
    ifstream inFile(filePath, ios::binary);
    h2_fm_model_t NewFlexModel;

    if (!inFile.is_open()) {
        cout << "Error: Could not open file " << filePath << endl;
        exit(1);
    }

    while (inFile.peek() != EOF) {
        h2_fm_blockheader_t blockHeader;
        inFile.read(blockHeader.ident, 32);
        inFile.read(reinterpret_cast<char*>(&blockHeader.version), sizeof(blockHeader.version));
        inFile.read(reinterpret_cast<char*>(&blockHeader.size), sizeof(blockHeader.size));

        cout << "Found Block: " << blockHeader.ident << " | Version: " << blockHeader.version << " | Size: " << blockHeader.size << " bytes" << endl;

        uint32_t curpos = inFile.tellg();

        if (string_view(blockHeader.ident) == FM_HEADER_NAME && blockHeader.version == FM_HEADER_VER) {
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.header.skinwidth), sizeof(NewFlexModel.header.skinwidth));
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.header.skinheight), sizeof(NewFlexModel.header.skinheight));
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.header.framesize), sizeof(NewFlexModel.header.framesize));
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.header.num_skins), sizeof(NewFlexModel.header.num_skins));
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.header.num_xyz), sizeof(NewFlexModel.header.num_xyz));
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.header.num_st), sizeof(NewFlexModel.header.num_st));
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.header.num_tris), sizeof(NewFlexModel.header.num_tris));
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.header.num_glcmds), sizeof(NewFlexModel.header.num_glcmds));
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.header.num_frames), sizeof(NewFlexModel.header.num_frames));
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.header.num_mesh_nodes), sizeof(NewFlexModel.header.num_mesh_nodes));
            cout << "Model Header parsed." << endl;
        }
        else if (string_view(blockHeader.ident) == FM_SKIN_NAME && blockHeader.version == FM_SKIN_VER) {
            for (int i = 0; i < NewFlexModel.header.num_skins; i++) {
                char newpath[64];
                inFile.read(newpath, 64);
                NewFlexModel.skins.emplace_back(newpath);
            }
            cout << "Skin block parsed." << endl;
        }
        else if (string_view(blockHeader.ident) == FM_ST_NAME && blockHeader.version == FM_ST_VER) {
            for (int i = 0; i < NewFlexModel.header.num_st; i++) {
                h2_fm_stvert_t newtexcord;
                inFile.read(reinterpret_cast<char*>(&newtexcord.s), sizeof(newtexcord.s));
                inFile.read(reinterpret_cast<char*>(&newtexcord.t), sizeof(newtexcord.t));
                NewFlexModel.uv.push_back(newtexcord);
            }
            cout << "UV block parsed." << endl;
        }
        else if (string_view(blockHeader.ident) == FM_TRI_NAME && blockHeader.version == FM_TRI_VER) {
            for (int i = 0; i < NewFlexModel.header.num_tris; i++) {
                h2_fm_triangle_t newtriangle;
                inFile.read(reinterpret_cast<char*>(&newtriangle.vertexIndices[0]), sizeof(newtriangle.vertexIndices[0]));
                inFile.read(reinterpret_cast<char*>(&newtriangle.vertexIndices[1]), sizeof(newtriangle.vertexIndices[1]));
                inFile.read(reinterpret_cast<char*>(&newtriangle.vertexIndices[2]), sizeof(newtriangle.vertexIndices[2]));
                inFile.read(reinterpret_cast<char*>(&newtriangle.textureIndices[0]), sizeof(newtriangle.textureIndices[0]));
                inFile.read(reinterpret_cast<char*>(&newtriangle.textureIndices[1]), sizeof(newtriangle.textureIndices[1]));
                inFile.read(reinterpret_cast<char*>(&newtriangle.textureIndices[2]), sizeof(newtriangle.textureIndices[2]));
                NewFlexModel.triangles.push_back(newtriangle);
            }
            cout << "Triangle block parsed." << endl;
        }
        else if (string_view(blockHeader.ident) == FM_FRAME_NAME && blockHeader.version == FM_FRAME_VER) {
            for (int i = 0; i < NewFlexModel.header.num_frames; i++) {
                h2_fm_frame_t newframe;
                inFile.read(reinterpret_cast<char*>(&newframe.scale[0]), sizeof(newframe.scale[0]));
                inFile.read(reinterpret_cast<char*>(&newframe.scale[1]), sizeof(newframe.scale[1]));
                inFile.read(reinterpret_cast<char*>(&newframe.scale[2]), sizeof(newframe.scale[2]));
                inFile.read(reinterpret_cast<char*>(&newframe.translate[0]), sizeof(newframe.translate[0]));
                inFile.read(reinterpret_cast<char*>(&newframe.translate[1]), sizeof(newframe.translate[1]));
                inFile.read(reinterpret_cast<char*>(&newframe.translate[2]), sizeof(newframe.translate[2]));
                inFile.read(newframe.name, 16);
                for (int j = 0; j < NewFlexModel.header.num_xyz; j++) {
                    h2_fm_triangleVertex_t newvert;
                    inFile.read(reinterpret_cast<char*>(&newvert.vertex[0]), sizeof(newvert.vertex[0]));
                    inFile.read(reinterpret_cast<char*>(&newvert.vertex[1]), sizeof(newvert.vertex[1]));
                    inFile.read(reinterpret_cast<char*>(&newvert.vertex[2]), sizeof(newvert.vertex[2]));
                    inFile.read(reinterpret_cast<char*>(&newvert.lightNormalIndex), sizeof(newvert.lightNormalIndex));
                    newframe.vertices.push_back(newvert);
                }
                NewFlexModel.frames.push_back(newframe);
            }
            cout << "Frame block parsed." << endl;
        }
        else if (string_view(blockHeader.ident) == FM_GLCMDS_NAME && blockHeader.version == FM_GLCMDS_VER) {
            for (int i = 0; i < NewFlexModel.header.num_mesh_nodes; i++) {
                while (true) {
                    int command;
                    inFile.read(reinterpret_cast<char*>(&command), sizeof(int));
                    
                    if (command == 0) break; // End of commands

                    h2_fm_glCommand_t cmd;
                    cmd.count = command;
                    int numVerts = abs(command);
                    for (int j = 0; j < numVerts; j++) {
                        h2_fm_glCommandVertex_t v;
                        inFile.read(reinterpret_cast<char*>(&v.s), sizeof(float));
                        inFile.read(reinterpret_cast<char*>(&v.t), sizeof(float));
                        inFile.read(reinterpret_cast<char*>(&v.vertexIndex), sizeof(int));
                        cmd.vertices.push_back(v);
                    }
                    NewFlexModel.glCommands.push_back(cmd);
                }
            }
            cout << "GL commands block parsed." << endl;
        }
        else if (string_view(blockHeader.ident) == FM_MESH_NAME && blockHeader.version == FM_MESH_VER) {
            for (int i = 0; i < NewFlexModel.header.num_mesh_nodes; i++) {
                h2_fm_meshnode_t newmeshnode;
                for (int j = 0; j < 256; j++) {
                    uint8_t newun;
                    inFile.read(reinterpret_cast<char*>(&newun), sizeof(newun));
                    newmeshnode.unused[j] = newun;
                }
                for (int j = 0; j < 256; j++) {
                    uint8_t newvert;
                    inFile.read(reinterpret_cast<char*>(&newvert), sizeof(newvert));
                    newmeshnode.verts[j] = newvert;
                }
                inFile.read(reinterpret_cast<char*>(&newmeshnode.start_glcmds), sizeof(newmeshnode.start_glcmds));
                inFile.read(reinterpret_cast<char*>(&newmeshnode.num_glcmds), sizeof(newmeshnode.num_glcmds));
                NewFlexModel.meshNodes.push_back(newmeshnode);
            }
            cout << "Mesh node block parsed." << endl;
        }
        else if (string_view(blockHeader.ident) == FM_SKELETON_NAME && blockHeader.version == FM_SKELETON_VER) {
            NewFlexModel.skeleton.isUsed = true;
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.skeleton.skeletalType), sizeof(int));
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.skeleton.numClusters), sizeof(int));
            if (NewFlexModel.skeleton.numClusters > 0) {
                vector<int> clusterVertCounts(NewFlexModel.skeleton.numClusters);
                for (int i = 0; i < NewFlexModel.skeleton.numClusters; i++)
                    inFile.read(reinterpret_cast<char*>(&clusterVertCounts[i]), sizeof(int));
                NewFlexModel.skeleton.clusters.resize(NewFlexModel.skeleton.numClusters);
                for (int i = 0; i < NewFlexModel.skeleton.numClusters; i++) {
                    NewFlexModel.skeleton.clusters[i].vertices.resize(clusterVertCounts[i]);
                    for (int v = 0; v < clusterVertCounts[i]; v++)
                        inFile.read(reinterpret_cast<char*>(&NewFlexModel.skeleton.clusters[i].vertices[v]), sizeof(int));
                }
            }

            inFile.read(reinterpret_cast<char*>(&NewFlexModel.skeleton.haveSkeleton), sizeof(int));
            if (NewFlexModel.skeleton.haveSkeleton != 0) {
                int numFrames = NewFlexModel.header.num_frames;
                NewFlexModel.skeleton.frames.resize(numFrames);
                for (int f = 0; f < numFrames; f++) {
                    NewFlexModel.skeleton.frames[f].joints.resize(NewFlexModel.skeleton.numClusters);
                    for (int c = 0; c < NewFlexModel.skeleton.numClusters; c++) {
                        h2_fm_Placement_t jointPlacement;
                        inFile.read(reinterpret_cast<char*>(jointPlacement.origin), sizeof(jointPlacement.origin));
                        inFile.read(reinterpret_cast<char*>(jointPlacement.direction), sizeof(jointPlacement.direction));
                        inFile.read(reinterpret_cast<char*>(jointPlacement.up), sizeof(jointPlacement.up));

                        NewFlexModel.skeleton.frames[f].joints[c] = jointPlacement;
                    }
                }
                cout << "Skeleton block parsed (" << NewFlexModel.skeleton.numClusters << " clusters across " << numFrames << " frames)." << endl;
            }
            else 
                cout << "Skeleton block parsed (" << NewFlexModel.skeleton.numClusters << " clusters, no frame joint transforms)." << endl;
        }
        else if (string_view(blockHeader.ident) == FM_REFERENCES_NAME && blockHeader.version == FM_REFERENCES_VER) {
            NewFlexModel.references.isUsed = true;
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.references.referenceType), sizeof(int));
            inFile.read(reinterpret_cast<char*>(&NewFlexModel.references.haveRefs), sizeof(int));
            if (NewFlexModel.references.haveRefs != 0) {
                size_t bytesRemaining = blockHeader.size - (sizeof(int) * 2);
                size_t count = bytesRemaining / sizeof(h2_fm_Placement_t);

                NewFlexModel.references.placements.resize(count);

                inFile.read(reinterpret_cast<char*>(NewFlexModel.references.placements.data()), bytesRemaining);
                cout << "Reference block parsed (" << count << " placements)." << endl;
            }
            else {
                cout << "Reference block parsed (haveRefs = 0)." << endl;
            }
        }
        else {
            cout << "Illegal block ident or version! Terminating program." << endl;
            exit(1);
        }

        if (static_cast<uint32_t>(inFile.tellg()) - curpos != blockHeader.size) {
            cout << "Block reading error; mismatch with block size: " << static_cast<uint32_t>(inFile.tellg()) - curpos << " vs " << blockHeader.size << " bytes" << endl;
            exit(1);
        }
    }

    inFile.close();
    return NewFlexModel;
}

void FM2JSON(h2_fm_model_t NewFlexModel, fs::path outPath) {
    cout << "Preparing JSON..." << endl;
     json jsonFM;
     jsonFM["Format"] = "FlexModel";

     json jheader;
     jheader["skinwidth"] = NewFlexModel.header.skinwidth;
     jheader["skinheight"] = NewFlexModel.header.skinheight;
     jheader["framesize"] = NewFlexModel.header.framesize;
     jheader["num_skins"] = NewFlexModel.header.num_skins;
     jheader["num_xyz"] = NewFlexModel.header.num_xyz;
     jheader["num_st"] = NewFlexModel.header.num_st;
     jheader["num_tris"] = NewFlexModel.header.num_tris;
     jheader["num_glcmds"] = NewFlexModel.header.num_glcmds;
     jheader["num_frames"] = NewFlexModel.header.num_frames;
     jheader["num_mesh_nodes"] = NewFlexModel.header.num_mesh_nodes;
     jsonFM["header"] = jheader;

     jsonFM["skins"] = json::array();;
     for (const auto& skinpath : NewFlexModel.skins) {
         json s;
         s = skinpath;
         jsonFM["skins"].push_back(s);
     }

     jsonFM["UV"] = json::array();;
     for (const auto& uve : NewFlexModel.uv) {
         json s;
         s = { uve.s, uve.t };
         jsonFM["UV"].push_back(s);
     }

     jsonFM["triangles"] = json::array();;
     for (const auto& tri : NewFlexModel.triangles) {
         json s;
         s = { tri.vertexIndices[0], tri.vertexIndices[1], tri.vertexIndices[2], tri.textureIndices[0], tri.textureIndices[1] , tri.textureIndices[2] };
         jsonFM["triangles"].push_back(s);
     }

     jsonFM["frames"] = json::array();
     for (const auto& frame : NewFlexModel.frames) {
         json jframe;
         jframe["scale"] = { frame.scale[0],frame.scale[1],frame.scale[2] };
         jframe["translate"] = { frame.translate[0], frame.translate[1], frame.translate[2] };
         jframe["name"] = frame.name;
         jframe["verts"] = json::array();
         for (int i = 0; i < NewFlexModel.header.num_xyz; i++) {
             json jvert;
             jvert = { frame.vertices[i].vertex[0],frame.vertices[i].vertex[1], frame.vertices[i].vertex[2], frame.vertices[i].lightNormalIndex };
             jframe["verts"].push_back(jvert);
         }
         jsonFM["frames"].push_back(jframe);

     }

     jsonFM["glCommands"] = json::array();
     for (const auto& cmd : NewFlexModel.glCommands) {
         json jCmd;
         jCmd["strip"] = (cmd.count > 0);
         jCmd["verts"] = json::array();
         for (const auto& v : cmd.vertices)
             jCmd["verts"].push_back({ v.s, v.t, v.vertexIndex });
         jsonFM["glCommands"].push_back(jCmd);
     }

     jsonFM["meshNodes"] = json::array();
     for (const auto& mn : NewFlexModel.meshNodes) {
         json jmn;
         jmn["unused"] = json::array();
         for (int i = 0; i < 256; i++)
             jmn["unused"].push_back(mn.unused[i]);
         jmn["verts"] = json::array();
         for (int i = 0; i < 256; i++)
             jmn["verts"].push_back(mn.verts[i]);
         jmn["start_glcmds"] = mn.start_glcmds;
         jmn["num_glcmds"] = mn.num_glcmds;
         jsonFM["meshNodes"].push_back(jmn);
     }

     json jskeleton;
     if(NewFlexModel.skeleton.isUsed){
         jskeleton["skeletalType"] = NewFlexModel.skeleton.skeletalType;
         jskeleton["numClusters"] = NewFlexModel.skeleton.numClusters;
         jskeleton["clusters"] = json::array();
         for (const auto& sc : NewFlexModel.skeleton.clusters) {
             json jcluster = json::array();
             for (const auto& v : sc.vertices)
                 jcluster.push_back(v);
             jskeleton["clusters"].push_back(jcluster);
         }
         jskeleton["haveSkeleton"] = (NewFlexModel.skeleton.haveSkeleton > 0);
         jskeleton["frames"] = json::array();
         for (const auto& sf : NewFlexModel.skeleton.frames) {
             json jsf = json::array();
             for (const auto& p : sf.joints) {
                 json jp;
                 jp = { p.origin[0], p.origin[1], p.origin[2], p.direction[0], p.direction[1], p.direction[2], p.up[0], p.up[1], p.up[2] };
                 jsf.push_back(jp);
             }
             jskeleton["frames"].push_back(jsf);
         }
     }
     jsonFM["skeleton"] = jskeleton;

     json jrefblock;
     if(NewFlexModel.references.isUsed){
         jrefblock["referenceType"] = NewFlexModel.references.referenceType;
         jrefblock["haveRefs"] = (NewFlexModel.references.haveRefs > 0);
         jrefblock["placements"] = json::array();
         for (const auto& p : NewFlexModel.references.placements) {
             json jp;
             jp = { p.origin[0], p.origin[1], p.origin[2], p.direction[0], p.direction[1], p.direction[2], p.up[0], p.up[1], p.up[2] };
             jrefblock["placements"].push_back(jp);
         }
     }
     jsonFM["references"] = jrefblock;

     cout << "JSON prepped, time to export." << endl;
     ofstream outFile(outPath);
     if (outFile.is_open()) {
         outFile << jsonFM.dump(2);
         outFile.close();
         cout << "JSON created successfully: " << outPath << endl;
     }
     else {
         cout << "Error: Could not write output file." << endl;
     }
}