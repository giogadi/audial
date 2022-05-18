#pragma once

class Entity;
class EntityManager;
class GameManager;

void LoadEntities(char const* filename, EntityManager& e, GameManager& g);

void SaveEntities(char const* filename, EntityManager& e);