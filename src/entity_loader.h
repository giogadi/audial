#pragma once

#include <iostream>

class Entity;
class EntityManager;
class GameManager;

bool LoadEntities(char const* filename, bool dieOnConnectFailure, EntityManager& e, GameManager& g);

void SaveEntities(char const* filename, EntityManager& e);

void SaveEntity(std::ostream& output, Entity const& e);

void LoadEntity(std::istream& input, Entity& e);