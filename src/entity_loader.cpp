#include "entity_loader.h"

#include <fstream>

#include "serialize.h"

void LoadEntities(char const* filename, EntityManager& e) {
    std::ifstream inFile(filename);
    cereal::JSONInputArchive archive(inFile);
    archive(e);
    e.ConnectComponents();
}

void SaveEntities(char const* filename, EntityManager& e) {
    std::ofstream outFile(filename);
    cereal::JSONOutputArchive archive(outFile);
    archive(CEREAL_NVP(e));
}