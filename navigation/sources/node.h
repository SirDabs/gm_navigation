/*
    gm_navigation
    By Spacetech
*/

#ifndef NODE_H
#define NODE_H

#include "eiface.h"
#include "defines.h"
#include "GarrysMod/Lua/Interface.h"
#include "GarrysMod/Interfaces.hpp"
#include <thread>

class DllExport Node
{
public:
	Node(const Vector &Position, const Vector &Norm, Node *Par);
	~Node();
	Node *GetParent();
	void ConnectTo(Node *node, NavDirType Dir);
	Node *GetConnectedNode(NavDirType Dir);
	const Vector *GetPosition();
	const Vector *GetNormal();
	void MarkAsVisited(NavDirType Dir);
	bool HasVisited(NavDirType Dir);
	
	// Stuff for AStar
	void SetStatus(Node* P, float F, float G);
	bool IsOpened();
	void SetOpened(bool Open);
	bool IsDisabled();
	void SetDisabled(bool bDisabled);
	bool IsClosed();
	void SetClosed(bool Close);
	float GetScoreF();
	float GetScoreG();
	int GetID();
	void SetID(int id);
	void SetAStarParent(Node* P);
	Node *GetAStarParent();
	void SetNormal(const Vector &Norm);
	void SetPosition(const Vector &Position);

	Vector vecPos;
	Vector vecNormal;

	void* customData;

private:
	int iID;
	Node *nodeParent;

	unsigned short visited;
	Node *connections[NUM_DIRECTIONS_MAX];

	Node *nodeAStarParent;
	bool bOpened;
	bool bClosed;
	bool bDisabled;
	float scoreF;
	float scoreG;
};

DllExport Node* LUA_GetNode(GarrysMod::Lua::ILuaBase* L, int Pos);
DllExport void LUA_PushNode(GarrysMod::Lua::ILuaBase* L, Node *node);

#endif
