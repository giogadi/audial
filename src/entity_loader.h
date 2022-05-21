#pragma once

class Entity;
class EntityManager;
class GameManager;

bool LoadEntities(char const* filename, EntityManager& e, GameManager& g);

void SaveEntities(char const* filename, EntityManager& e);