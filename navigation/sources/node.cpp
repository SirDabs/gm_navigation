/*
    gm_navigation
    By Spacetech
*/


#define GMOD_USE_SOURCESDK
#define GMOD_ALLOW_DEPRECATED

#include "node.h"
#include "tier0/memdbgon.h"
#include <GarrysMod/Lua/UserData.h>

using namespace GarrysMod::Lua;

Node* LUA_GetNode(GarrysMod::Lua::ILuaBase* L, int pos)
{
    L->CheckType(pos, NODE_TYPE);
	UserData *pUserData = static_cast<UserData*>(L->GetUserdata(pos));
	if(pUserData == nullptr)
	{
		L->ArgError(pos, "Invalid Node Err 1: ");
	}

	Node *pNode = reinterpret_cast<Node*>(pUserData->data);
	if(pNode == nullptr)
	{
		L->ArgError(pos, "Invalid Node Err 2: ");
	}

	return pNode;
}

void LUA_PushNode(GarrysMod::Lua::ILuaBase* L, Node *node)
{
	if(node)
	{
		UserData *data = (UserData*)L->NewUserdata(sizeof(UserData));
        data->data = (void*)node;
        data->type = NODE_TYPE;
        
        L->CreateMetaTable( NODE_NAME );
        L->SetMetaTable(-2);
	}
	else
	{
		L->PushBool(false);
	}
}

Node::Node(const Vector &Position, const Vector &Norm, Node *Par) :
    iID(-1), vecPos(Position), vecNormal(Norm), nodeParent(Par), visited(0),
    bOpened(false), bClosed(false), bDisabled(false), nodeAStarParent(NULL), scoreF(0), scoreG(0), customData(NULL)
{
	// Not sure if theres any memory problems with this.
	// If it's not diagonal your still setting 4 of them to NULL
	for(int i = NORTH; i < NUM_DIRECTIONS_MAX; i++)
	{
		connections[i] = NULL;
	}
}

Node::~Node()
{
	if(customData != NULL)
	{
		delete customData;
		customData = NULL;
	}
}

void Node::ConnectTo(Node *node, NavDirType Dir)
{
	connections[Dir] = node;
}

Node *Node::GetConnectedNode(NavDirType Dir)
{
	return connections[Dir];
}

const Vector *Node::GetPosition()
{
	return &vecPos;
}

const Vector *Node::GetNormal()
{
	return &vecNormal;
}

Node *Node::GetParent()
{
	return nodeParent;
}

void Node::MarkAsVisited(NavDirType Dir)
{
	visited |= (1 << Dir);
}

bool Node::HasVisited(NavDirType Dir)
{
	if(visited & (1 << Dir))
	{
		return true;
	}
	return false;
}

void Node::SetStatus(Node* P, float F, float G)
{
	nodeAStarParent = P;
	scoreF = F;
	scoreG = G;
}

bool Node::IsOpened()
{
	return bOpened;
}

void Node::SetOpened(bool Open)
{
	bOpened = Open;
}

bool Node::IsDisabled()
{
	return bDisabled;
}

void Node::SetDisabled(bool bDisabled)
{
	this->bDisabled = bDisabled;
}

bool Node::IsClosed()
{
	return bClosed;
}

void Node::SetClosed(bool Close)
{
	bClosed = Close;
}

Node *Node::GetAStarParent()
{
	return nodeAStarParent;
}

void Node::SetAStarParent(Node* P)
{
	nodeAStarParent = P;
}

float Node::GetScoreF()
{
	return scoreF;
}

float Node::GetScoreG()
{
	return scoreG;
}

int Node::GetID()
{
	return iID;
}

void Node::SetID(int id)
{
	iID = id;
}

void Node::SetNormal(const Vector &Norm)
{
	vecNormal = Norm;
}

void Node::SetPosition(const Vector &Position)
{
	vecPos = Position;
}
