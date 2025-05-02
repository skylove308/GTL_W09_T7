#include "FBXLoader.h"
#include <iostream>
#include "Define.h"
void FFBXLoader::LoadFBX(const char* fileName)
{
    FbxManager* manager = FbxManager::Create();

    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ios);
    FbxImporter* importer = FbxImporter::Create(manager, "");
    if (!importer->Initialize(fileName, -1, manager->GetIOSettings()))
    {
        importer->Destroy();
        manager->Destroy();
        return;
    }

    FbxScene* scene = FbxScene::Create(manager, "My Scene");
    importer->Import(scene);
    importer->Destroy();

    FbxNode* rootNode = scene->GetRootNode();
    if (rootNode)
    {
        for (int i = 0; i < rootNode->GetChildCount(); i++)
        {
            FbxNode* child = rootNode->GetChild(i);
            std::cout << "Node Name :" << child->GetName() << std::endl;
            UE_LOG(ELogLevel::Display, "Node Name : %s", child->GetName());
        }
    }

    scene->Destroy();
    manager->Destroy();
}
