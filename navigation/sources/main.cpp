/*
    gm_navigation
    By Spacetech
*/

#define GMOD_USE_SOURCESDK
#define GMOD_ALLOW_DEPRECATED
#define LUA_NOREF       (-2)
#define LUA_REFNIL      (-1)
#define IS_SERVERSIDE true


#include "main.h"

#include "nav.h"
#include "node.h"
#include "eiface.h"
#include "cdll_int.h"
#include "engine/IEngineTrace.h"
#include "tier0/memdbgon.h"


const std::string engine_lib = Helpers::GetBinaryFileName("engine",
                                                          false,
                                                          IS_SERVERSIDE,
                                                          "bin/");

SourceSDK::FactoryLoader engine_loader( engine_lib, false, false );
CreateInterfaceFn engine_factory = nullptr;

IVEngineServer *engine_server = nullptr;
IVEngineClient *engine_client = nullptr;
IEngineTrace *engine_trace = nullptr;
IServer *server = nullptr;
bool is_dedicated = true;

typedef CUtlVector<Node*> NodeList_t;

CUtlVector<JobInfo_t*> JobQueue;

using namespace GarrysMod::Lua;

LUA_FUNCTION(nav_Create)
{
	LUA->CheckType(1, Type::NUMBER);

	LUA_PushNav(LUA, new Nav((int)LUA->GetNumber(1)));

	return 1;
}

#ifdef USE_BOOST_THREADS
boost::posix_time::time_duration timeout = boost::posix_time::milliseconds(0);
boost::posix_time::time_duration sleep_time = boost::posix_time::milliseconds(50);
#endif

LUA_FUNCTION(nav_Poll)
{
	for(int i=0; i < JobQueue.Size(); i++)
	{
		JobInfo_t* info = JobQueue[i];
#ifdef USE_BOOST_THREADS
		if(info->finished) // || info->thread.timed_join(timeout))
#else
		if(info->job->IsFinished())
#endif
		{
#ifdef FILEBUG
			FILEBUG_WRITE("Job Finished\n");
#endif

            LUA->ReferencePush(info->luaFuncRef);
            
#ifdef FILEBUG
			FILEBUG_WRITE("Pushed Reference\n");
#endif

			if(LUA->GetType(-1) != Type::FUNCTION)
			{
				LUA->Pop();
				info->finished = true;
#ifdef FILEBUG
				FILEBUG_WRITE("Not a function!?\n");
#endif

				// Emergency abort :/
				delete info;
				JobQueue.Remove(i);

				i = 0;

				continue;
			}

#ifdef FILEBUG
			FILEBUG_WRITE("Pushing nav\n");
#endif

			LUA_PushNav(LUA, info->nav);

#ifdef FILEBUG
			FILEBUG_WRITE("Pushed nav\n");
#endif

			if(info->findPath)
			{
#ifdef FILEBUG
				FILEBUG_WRITE("Find Path\n");
#endif

				LUA->Push(info->foundPath);

                LUA->CreateTable();

				for(int i = 0; i < info->path.Count(); i++)
                {
                    LUA->PushNumber(i + 1);
					LUA_PushNode(LUA, info->path[i]);
                    LUA->SetTable(-3);
				}

#ifdef FILEBUG
				FILEBUG_WRITE("Calling Callback 1\n");

				for(int i=0; i <= LUA->Top(); i++)
				{
					FILEBUG_WRITE("Stack: %d | Type: %d | Type Name: %s\n", i, LUA->GetType(i), LUA->GetTypeName(LUA->GetType(i)));
					if(LUA->GetType(i) == Type::NUMBER)
					{
						FILEBUG_WRITE("\t%f\n", LUA->GetNumber(i));
					}
                }
#endif

				LUA->Call(3, 0);
                

#ifdef FILEBUG
				FILEBUG_WRITE("Calling Callback 2\n");
#endif
			}
			else
			{
#ifdef FILEBUG
				FILEBUG_WRITE("Calling Callback 3\n");

				for(int i=0; i <= LUA->Top(); i++)
				{
					FILEBUG_WRITE("Stack: %d | Type: %d | Type Name: %s\n", i, LUA->GetType(i), LUA->GetTypeName(LUA->GetType(i)));
					if(LUA->GetType(i) == Type::NUMBER)
					{
						FILEBUG_WRITE("\t%f\n", LUA->GetNumber(i));
					}
				}
#endif

				LUA->Call(1, 0);

#ifdef FILEBUG
				FILEBUG_WRITE("Calling Callback 4\n");
#endif
			}

#ifdef FILEBUG
			FILEBUG_WRITE("Freeing Reference\n");
#endif

            LUA->ReferenceFree(info->luaFuncRef);
            
			if(info->luaUpdateFuncRef != LUA_NOREF)
			{
                LUA->ReferenceFree(info->luaUpdateFuncRef);
                info->luaUpdateFuncRef = LUA_NOREF;
			}

#ifndef USE_BOOST_THREADS
			SafeRelease(info->job);
#endif

			delete info;
			JobQueue.Remove(i);

			i = 0;

#ifdef FILEBUG
			FILEBUG_WRITE("Job Completely Finished\n");
#endif
		}
		else if(!info->findPath && info->updateTime > 0)
		{
            if(info->luaUpdateFuncRef != LUA_NOREF) {
                time_t now = time(NULL);

                if(difftime(now, info->updateTime) >= 1)
                {
                    LUA->ReferencePush(info->luaUpdateFuncRef);
					LUA_PushNav(LUA, info->nav);
                    LUA->PushNumber(info->nav->GetNodes().Size());
                    LUA->Call(2, 0);

                    info->updateTime = now;
                }
            }
		}
	}

	return 0;
}

///////////////////////////////////////////////

LUA_FUNCTION(Nav_GetNodeByID)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::NUMBER);

	LUA_PushNode(LUA, LUA_GetNav(LUA, 1)->GetNodeByID((int)LUA->GetNumber(2) - 1));

	return 1;
}

LUA_FUNCTION(Nav_GetNodeTotal)
{
	LUA->CheckType(1, NAV_TYPE);

	LUA->PushNumber(LUA_GetNav(LUA, 1)->GetNodes().Count());

	return 1;
}

LUA_FUNCTION(Nav_GetNodes)
{
	LUA->CheckType(1, NAV_TYPE);

	NodeList_t& Nodes = LUA_GetNav(LUA, 1)->GetNodes();

    LUA->CreateTable();

	for(int i = 0; i < Nodes.Count(); i++)
    {
        LUA->PushNumber(i + 1);
		LUA_PushNode(LUA, Nodes[i]);
        LUA->SetTable(-3);
	}

	return 1;
}

LUA_FUNCTION(Nav_GetNearestNodes)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::VECTOR);
	LUA->CheckType(3, Type::NUMBER);

	Vector pos = LUA_GetVector(LUA, 2);

	int count = 1;
	double pt[3] = { pos.x, pos.y, pos.z };

	Nav *nav = LUA_GetNav(LUA, 1);
	
    kdres *results = kd_nearest_range(nav->GetNodeTree(), pt, LUA->GetNumber(3));
    
    LUA->CreateTable();

	while(results != NULL && !kd_res_end(results))
    {
        LUA->PushNumber(count++);
		LUA_PushNode(LUA, (Node*)kd_res_item_data(results));
        LUA->SetTable(-3);

		kd_res_next(results);
	}
	
	if(results != NULL)
	{
		kd_res_free(results);
	}

	return 1;
}

LUA_FUNCTION(Nav_ResetGeneration)
{
	LUA->CheckType(1, NAV_TYPE);

	LUA_GetNav(LUA, 1)->ResetGeneration();

	return 0;
}

LUA_FUNCTION(Nav_SetupMaxDistance)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::VECTOR);
	LUA->CheckType(3, Type::NUMBER);

	LUA_GetNav(LUA, 1)->SetupMaxDistance(LUA_GetVector(LUA, 2), LUA->GetNumber(3));

	return 0;
}

LUA_FUNCTION(Nav_AddGroundSeed)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::VECTOR);
	LUA->CheckType(3, Type::VECTOR);

	LUA_GetNav(LUA, 1)->AddGroundSeed(LUA_GetVector(LUA, 2), LUA_GetVector(LUA, 3));

	return 0;
}

LUA_FUNCTION(Nav_AddAirSeed)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::VECTOR);

	LUA_GetNav(LUA, 1)->AddAirSeed(LUA_GetVector(LUA, 2));

	return 0;
}

LUA_FUNCTION(Nav_ClearGroundSeeds)
{
	LUA->CheckType(1, NAV_TYPE);

	LUA_GetNav(LUA, 1)->ClearGroundSeeds();

	return 0;
}

LUA_FUNCTION(Nav_ClearAirSeeds)
{
	LUA->CheckType(1, NAV_TYPE);

	LUA_GetNav(LUA, 1)->ClearAirSeeds();

	return 0;
}

LUA_FUNCTION(Nav_Generate)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::FUNCTION);
	
	Nav *nav = LUA_GetNav(LUA, 1);

	JobInfo_t *info = new JobInfo_t;
	info->nav = nav;
	info->abort = false;
	info->finished = false;
	info->findPath = false;
    
    LUA->Push(2);
    info->luaFuncRef = LUA->ReferenceCreate();

	if(LUA->GetType(3) == Type::FUNCTION)
	{
        LUA->Push(3);
        info->luaUpdateFuncRef = LUA->ReferenceCreate();
		info->updateTime = time(NULL);
	}
	else
	{
        info->luaUpdateFuncRef = LUA_NOREF;
		info->updateTime = 0;
	}

	nav->GenerateQueue(info);

#ifndef USE_BOOST_THREADS
	if(info->job != NULL)
	{
#endif
		JobQueue.AddToTail(info);
#ifndef USE_BOOST_THREADS
	}
	else
	{
#ifdef FILEBUG
		FILEBUG_WRITE("Invalid job!\n");
#endif
		// free ref info->updateRef
		delete info;
	}
	LUA->PushBool(info->job != NULL);
#else
	LUA->PushBool(true);
#endif

#ifdef FILEBUG
	FILEBUG_WRITE("Nav_Generate\n");
#endif

	return 1;
}

LUA_FUNCTION(Nav_FullGeneration)
{
	LUA->CheckType(1, NAV_TYPE);

	Nav *nav = LUA_GetNav(LUA, 1);

	float StartTime = engine_server->Time();

	nav->ResetGeneration();
	nav->FullGeneration(NULL);

	LUA->Push(engine_server->Time() - StartTime);

	return 1;
}

LUA_FUNCTION(Nav_IsGenerated)
{
	LUA->CheckType(1, NAV_TYPE);

	LUA->Push(LUA_GetNav(LUA, 1)->IsGenerated());

	return 1;
}

LUA_FUNCTION(Nav_FindPath)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::FUNCTION);

	Nav* nav = LUA_GetNav(LUA, 1);

	JobInfo_t *info = new JobInfo_t;
	info->nav = nav;
	info->abort = false;
	info->finished = false;
	info->findPath = true;
	info->hull = false;
	info->diagonal = nav->GetDiagonal();
	info->heuristic = nav->GetHeuristic();
    
    LUA->Push(2);
    info->luaFuncRef = LUA->ReferenceCreate();
	info->luaUpdateFuncRef = LUA_NOREF;

	nav->FindPathQueue(info);

#ifndef USE_BOOST_THREADS
	if(info->job != NULL)
	{
#endif
		JobQueue.AddToTail(info);
#ifndef USE_BOOST_THREADS
	}
	else
	{
		// free ref info->funcRef
		delete info;
	}

	LUA->PushBool((info->job != NULL));
#else
	LUA->PushBool(true);
#endif

	return 1;
}

LUA_FUNCTION(Nav_FindPathImmediate)
{
	LUA->CheckType(1, NAV_TYPE);

	Nav* nav = LUA_GetNav(LUA, 1);

	JobInfo_t *info = new JobInfo_t;
	info->nav = nav;
	info->abort = false;
	info->finished = false;
	info->findPath = true;
	info->hull = false;
	info->diagonal = nav->GetDiagonal();
	info->heuristic = nav->GetHeuristic();

	nav->ExecuteFindPath(info, nav->GetStart(), nav->GetEnd());

	if(info->foundPath)
	{
        LUA->CreateTable();
        
		for(int i = 0; i < info->path.Count(); i++)
		{
            LUA->PushNumber(i + 1);
			LUA_PushNode(LUA, info->path[i]);
            LUA->SetTable(-3);
		}
	}
	else
	{
		LUA->PushBool(false);
	}

	delete info;

	return 1;
}

LUA_FUNCTION(Nav_FindPathHull)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::VECTOR);
	LUA->CheckType(3, Type::VECTOR);
	LUA->CheckType(4, Type::FUNCTION);

	Nav* nav = LUA_GetNav(LUA, 1);

	JobInfo_t *info = new JobInfo_t;
	info->nav = nav;
	info->abort = false;
	info->finished = false;
	info->findPath = true;
	info->hull = true;
	info->diagonal = nav->GetDiagonal();
	info->heuristic = nav->GetHeuristic();
	info->mins = LUA_GetVector(LUA, 2);
	info->maxs = LUA_GetVector(LUA, 3);
    
    LUA->Push(4);
    info->luaFuncRef = LUA->ReferenceCreate();
    info->luaUpdateFuncRef = LUA_NOREF;

	nav->FindPathQueue(info);

#ifndef USE_BOOST_THREADS
	if(info->job != NULL)
	{
#endif
		JobQueue.AddToTail(info);
#ifndef USE_BOOST_THREADS
	}
	else
	{
		// free ref info->funcRef
		delete info;
	}

	LUA->PushBool(info->job != NULL);
#else
	LUA->Push(true);
#endif

	return 1;
}

LUA_FUNCTION(Nav_FindPathHullImmediate)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::VECTOR);
	LUA->CheckType(3, Type::VECTOR);

	Nav* nav = LUA_GetNav(LUA, 1);

	JobInfo_t *info = new JobInfo_t;
	info->nav = nav;
	info->abort = false;
	info->finished = false;
	info->findPath = true;
	info->hull = true;
	info->diagonal = nav->GetDiagonal();
	info->heuristic = nav->GetHeuristic();
	info->mins = LUA_GetVector(LUA, 2);
	info->maxs = LUA_GetVector(LUA, 3);

	nav->ExecuteFindPath(info, nav->GetStart(), nav->GetEnd());

	if(info->foundPath)
	{
        LUA->CreateTable();

		for(int i = 0; i < info->path.Count(); i++)
		{
            LUA->PushNumber(i + 1);
			LUA_PushNode(LUA, info->path[i]);
            LUA->SetTable(-3);
		}
	}
	else
	{
		LUA->PushBool(false);
	}

	delete info;

	return 1;
}

LUA_FUNCTION(Nav_GetHeuristic)
{
	LUA->CheckType(1, NAV_TYPE);

	LUA->PushNumber(LUA_GetNav(LUA, 1)->GetHeuristic());

	return 1;
}

LUA_FUNCTION(Nav_GetStart)
{
	LUA->CheckType(1, NAV_TYPE);

	LUA_PushNode(LUA, LUA_GetNav(LUA, 1)->GetStart());

	return 1;
}

LUA_FUNCTION(Nav_GetEnd)
{
	LUA->CheckType(1, NAV_TYPE);

	LUA_PushNode(LUA, LUA_GetNav(LUA, 1)->GetEnd());

	return 1;
}

LUA_FUNCTION(Nav_SetHeuristic)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::NUMBER);

	LUA_GetNav(LUA, 1)->SetHeuristic(LUA->GetNumber(2));

	return 0;
}

LUA_FUNCTION(Nav_SetStart)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, NODE_TYPE);

	LUA_GetNav(LUA, 1)->SetStart(LUA_GetNode(LUA, 2));

	return 0;
}

LUA_FUNCTION(Nav_SetEnd)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, NODE_TYPE);

	LUA_GetNav(LUA, 1)->SetEnd(LUA_GetNode(LUA, 2));

	return 0;
}

LUA_FUNCTION(Nav_GetNode)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::VECTOR);

	LUA_PushNode(LUA, LUA_GetNav(LUA, 1)->GetNode(LUA_GetVector(LUA, 2)));

	return 1;
}

LUA_FUNCTION(Nav_GetClosestNode)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::VECTOR);

	LUA_PushNode(LUA, LUA_GetNav(LUA, 1)->GetClosestNode(LUA_GetVector(LUA, 2)));

	return 1;
}

LUA_FUNCTION(Nav_Save)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::STRING);

	LUA->PushBool(LUA_GetNav(LUA, 1)->Save(LUA->GetString(2)));

	return 1;
}

LUA_FUNCTION(Nav_Load)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::STRING);

	LUA->PushBool(LUA_GetNav(LUA, 1)->Load(LUA->GetString(2)));

	return 1;
}

LUA_FUNCTION(Nav_GetDiagonal)
{
	LUA->CheckType(1, NAV_TYPE);

	LUA->PushBool(LUA_GetNav(LUA, 1)->GetDiagonal());

	return 1;
}

LUA_FUNCTION(Nav_SetDiagonal)
{
	LUA->CheckType(1, NAV_TYPE);

	LUA_GetNav(LUA, 1)->SetDiagonal(LUA->GetBool(2));

	return 0;
}

LUA_FUNCTION(Nav_GetGridSize)
{
	LUA->CheckType(1, NAV_TYPE);
	
	LUA->PushNumber(LUA_GetNav(LUA, 1)->GetGridSize());

	return 1;
}

LUA_FUNCTION(Nav_SetGridSize)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::NUMBER);

	LUA_GetNav(LUA, 1)->SetGridSize((int)LUA->GetNumber(2));

	return 0;
}

LUA_FUNCTION(Nav_GetMask)
{
	LUA->CheckType(1, NAV_TYPE);
	
	LUA->PushNumber(LUA_GetNav(LUA, 1)->GetMask());

	return 1;
}

LUA_FUNCTION(Nav_SetMask)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::NUMBER);

	LUA_GetNav(LUA, 1)->SetMask((int)LUA->GetNumber(2));

	return 0;
}

LUA_FUNCTION(Nav_CreateNode)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, Type::VECTOR);
	LUA->CheckType(3, Type::VECTOR);

	LUA_PushNode(LUA, LUA_GetNav(LUA, 1)->AddNode(LUA_GetVector(LUA, 2), LUA_GetVector(LUA, 3), NORTH, NULL)); // This dir doesn't matter

	return 1;
}

LUA_FUNCTION(Nav_RemoveNode)
{
	LUA->CheckType(1, NAV_TYPE);
	LUA->CheckType(2, NODE_TYPE);

	LUA_GetNav(LUA, 1)->RemoveNode(LUA_GetNode(LUA, 2));

	return 0;
}

///////////////////////////////////////////////

LUA_FUNCTION(Node__eq)
{
	LUA->CheckType(1, NODE_TYPE);
	LUA->CheckType(2, NODE_TYPE);

	LUA->PushBool(LUA_GetNode(LUA, 1)->GetID() == LUA_GetNode(LUA, 2)->GetID());

	return 1;
}

LUA_FUNCTION(Node_GetID)
{
	LUA->CheckType(1, NODE_TYPE);

	LUA->PushNumber(LUA_GetNode(LUA, 1)->GetID() + 1);

	return 1;
}

LUA_FUNCTION(Node_GetPosition)
{
	LUA->CheckType(1, NODE_TYPE);

	LUA_PushVector(LUA, LUA_GetNode(LUA, 1)->vecPos);

	return 1;
}

LUA_FUNCTION(Node_GetNormal)
{
	LUA->CheckType(1, NODE_TYPE);

	LUA_PushVector(LUA, LUA_GetNode(LUA, 1)->vecNormal);

	return 1;
}

LUA_FUNCTION(Node_IsDisabled)
{
	LUA->CheckType(1, NODE_TYPE);

	LUA->Push(LUA_GetNode(LUA, 1)->IsDisabled());

	return 1;
}

LUA_FUNCTION(Node_SetDisabled)
{
	LUA->CheckType(1, NODE_TYPE);
	LUA->CheckType(2, Type::BOOL);

	LUA_GetNode(LUA, 1)->SetDisabled(LUA->GetBool(2));

	return 0;
}

LUA_FUNCTION(Node_GetConnections)
{
	LUA->CheckType(1, NODE_TYPE);

	Node *node = LUA_GetNode(LUA, 1);

    LUA->CreateTable();
	
	Node *Connection;

	for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++)
	{
		Connection = node->GetConnectedNode((NavDirType)Dir);
		if(Connection != NULL)
		{
            LUA->PushNumber(Dir);
			LUA_PushNode(LUA, Connection);
            LUA->SetTable(-3);
		}
	}


	return 1;
}

LUA_FUNCTION(Node_GetConnectedNode)
{
	LUA->CheckType(1, NODE_TYPE);
	LUA->CheckType(2, Type::NUMBER);

	int dir = (int)LUA->GetNumber(2);

	if(dir >= 0 && dir < NUM_DIRECTIONS_MAX)
	{
		Node *pNode = LUA_GetNode(LUA, 1);

		Node *pNodeConnection = pNode->GetConnectedNode((NavDirType)dir);
		if(pNodeConnection != NULL)
		{
			LUA_PushNode(LUA, pNodeConnection);
			return 1;
		}
	}

	LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(Node_GetScoreG)
{
	LUA->CheckType(1, NODE_TYPE);

	LUA->PushNumber(LUA_GetNode(LUA, 1)->GetScoreG());

	return 1;
}

LUA_FUNCTION(Node_GetScoreF)
{
	LUA->CheckType(1, NODE_TYPE);

	LUA->PushNumber(LUA_GetNode(LUA, 1)->GetScoreF());

	return 1;
}

LUA_FUNCTION(Node_IsConnected)
{
	LUA->CheckType(1, NODE_TYPE);
	LUA->CheckType(2, NODE_TYPE);

	Node *Node1 = LUA_GetNode(LUA, 1);
	Node *Node2 = LUA_GetNode(LUA, 2);

	Node *Connection;
	for(int Dir = NORTH; Dir < NUM_DIRECTIONS_MAX; Dir++)
	{
		Connection = Node1->GetConnectedNode((NavDirType)Dir);
		if(Connection != NULL && Connection == Node2)
		{
			LUA->PushBool(true);

			return 1;
		}
	}

	LUA->PushBool(false);

	return 1;
}

LUA_FUNCTION(Node_SetNormal)
{
	LUA->CheckType(1, NODE_TYPE);
	LUA->CheckType(2, Type::VECTOR);

	LUA_GetNode(LUA, 1)->SetNormal(LUA_GetVector(LUA, 2));

	return 0;
}

LUA_FUNCTION(Node_SetPosition)
{
	LUA->CheckType(1, NODE_TYPE);
	LUA->CheckType(2, Type::VECTOR);

	LUA_GetNode(LUA, 1)->SetPosition(LUA_GetVector(LUA, 2));

	return 0;
}

LUA_FUNCTION(Node_ConnectTo)
{
	LUA->CheckType(1, NODE_TYPE);
	LUA->CheckType(2, NODE_TYPE);
	LUA->CheckType(3, Type::NUMBER);

	Node *Node1 = LUA_GetNode(LUA, 1);
	Node *Node2 = LUA_GetNode(LUA, 2);
	NavDirType Dir = (NavDirType)(int)LUA->GetNumber(3);

	Node1->ConnectTo(Node2, Dir);
	Node2->ConnectTo(Node1, Nav::OppositeDirection(Dir));

	Node1->MarkAsVisited(Dir);
	Node1->MarkAsVisited(Nav::OppositeDirection(Dir));

	Node2->MarkAsVisited(Dir);

	return 0;
}

LUA_FUNCTION(Node_RemoveConnection)
{
	LUA->CheckType(1, NODE_TYPE);
	LUA->CheckType(2, Type::NUMBER);

	Node *Node1 = LUA_GetNode(LUA, 1);
	NavDirType Dir = (NavDirType)(int)LUA->GetNumber(2);

	Node *Node2 = Node1->GetConnectedNode(Dir);
	if(Node2 != NULL)
	{
		Node1->ConnectTo(NULL, Dir);
		Node2->ConnectTo(NULL, Nav::OppositeDirection(Dir));

		// UnMarkAsVisited?
		// I don't really know bitwise too well
	}

	return 0;
}

///////////////////////////////////////////////

LUA_FUNCTION( AddPollHook ) 
{
    // TODO: Figure out why this give linker errors
    //LUA->PushCFunction( helpers::LuaErrorTraceback );
    
    LUA->GetField( INDEX_GLOBAL, "hook" );
    if( !LUA->IsType( -1, Type::TABLE ) )
    {
        LUA->Pop( 2 );
        return false;
    }
    
    LUA->GetField( -1, "Add" );
    if( !LUA->IsType( -1, Type::FUNCTION ) )
    {
        LUA->Pop( 3 );
        return false;
    }
    
    LUA->Remove( -2 );
    
    LUA->PushString( "Think" );
    LUA->PushString( "NavPoll" );
    LUA->PushCFunction(nav_Poll);
    
    LUA->Call(3, 0);
    
    return true;
}

GMOD_MODULE_OPEN()
{
    engine_factory = engine_loader.GetFactory( );
    if( engine_factory == nullptr )
        LUA->ThrowError( "gm_navigation: failed to retrieve engine factory function\n" );
    
    engine_server = static_cast<IVEngineServer *>(engine_factory( INTERFACEVERSION_VENGINESERVER_VERSION_21, nullptr ));
    if( engine_server == nullptr )
        LUA->ThrowError( "gm_navigation: failed to retrieve server engine interface\n" );
    
    is_dedicated = engine_server->IsDedicatedServer( );
    if( !is_dedicated )
    {
        engine_client = static_cast<IVEngineClient *>(engine_factory( "VEngineClient015", nullptr ));
        if( engine_client == nullptr )
            LUA->ThrowError( "failed to retrieve client engine interface\n" );
    }

#ifdef IS_SERVERSIDE
	engine_trace = (IEngineTrace*)engine_factory( INTERFACEVERSION_ENGINETRACE_SERVER, nullptr );
#else
	engine_trace = (IEngineTrace*)engine_factory( INTERFACEVERSION_ENGINETRACE_CLIENT, nullptr );
#endif
	
	if( engine_trace == nullptr )
	{
		LUA->ThrowError("gm_navigation: Missing IEngineTrace interface\n");
	}

#ifndef USE_BOOST_THREADS
	threadPool = V_CreateThreadPool();

	ThreadPoolStartParams_t params;
	params.nThreads = 2;

	if(!threadPool->Start(params))
	{
		SafeRelease(threadPool);
		LUA->ThrowError("gm_navigation: Thread pool is null");
	}
#endif

    LUA->CreateTable();
    
    LUA->PushCFunction( nav_Create );
    LUA->SetField( -2, "Create" );
    LUA->PushCFunction( nav_Poll );
    LUA->SetField( -2, "Poll" );
    
    LUA->PushNumber( NORTH );
    LUA->SetField( -2, "NORTH" );
    LUA->PushNumber( SOUTH );
    LUA->SetField( -2, "SOUTH" );
    LUA->PushNumber( EAST );
    LUA->SetField( -2, "EAST" );
    LUA->PushNumber( WEST );
    LUA->SetField( -2, "WEST" );
    LUA->PushNumber( NORTHEAST );
    LUA->SetField( -2, "NORTHEAST" );
    LUA->PushNumber( NORTHWEST );
    LUA->SetField( -2, "NORTHWEST" );
    LUA->PushNumber( SOUTHEAST );
    LUA->SetField( -2, "SOUTHEAST" );
    LUA->PushNumber( SOUTHWEST );
    LUA->SetField( -2, "SOUTHWEST" );
    LUA->PushNumber( UP );
    LUA->SetField( -2, "UP" );
    LUA->PushNumber( DOWN );
    LUA->SetField( -2, "DOWN" );
    LUA->PushNumber( LEFT );
    LUA->SetField( -2, "LEFT" );
    LUA->PushNumber( RIGHT );
    LUA->SetField( -2, "RIGHT" );
    LUA->PushNumber( FORWARD);
    LUA->SetField( -2, "FORWARD" );
    LUA->PushNumber( BACKWARD );
    LUA->SetField( -2, "BACKWARD" );
    
    LUA->PushNumber( NUM_DIRECTIONS );
    LUA->SetField( -2, "NUM_DIRECTIONS" );
    LUA->PushNumber( NUM_DIRECTIONS_DIAGONAL );
    LUA->SetField( -2, "NUM_DIRECTIONS_DIAGONAL" );
    LUA->PushNumber( NUM_DIRECTIONS_MAX );
    LUA->SetField( -2, "NUM_DIRECTIONS_MAX" );
    
    LUA->PushNumber( Nav::HEURISTIC_MANHATTAN );
    LUA->SetField( -2, "HEURISTIC_MANHATTAN" );
    LUA->PushNumber( Nav::HEURISTIC_EUCLIDEAN );
    LUA->SetField( -2, "HEURISTIC_EUCLIDEAN" );
    LUA->PushNumber( Nav::HEURISTIC_CUSTOM );
    LUA->SetField( -2, "HEURISTIC_CUSTOM" );

    LUA->SetField(INDEX_GLOBAL, "nav");
    
    LUA->Pop();

    LUA->CreateMetaTableType(NAV_NAME, NAV_TYPE);
    LUA->CreateTable();
    
    LUA->PushCFunction( Nav_GetNodeByID );
    LUA->SetField( -2, "GetNodeByID" );
    LUA->PushCFunction( Nav_GetNodes );
    LUA->SetField( -2, "GetNodes" );
    LUA->PushCFunction( Nav_GetNearestNodes );
    LUA->SetField( -2, "GetNearestNodes" );
    
    LUA->PushCFunction( Nav_GetNodeTotal );
    LUA->SetField( -2, "GetNodeTotal" );
    
    LUA->PushCFunction( Nav_AddGroundSeed );
    LUA->SetField( -2, "AddGroundSeed" );
    LUA->PushCFunction( Nav_AddAirSeed );
    LUA->SetField( -2, "AddAirSeed" );
    
    LUA->PushCFunction( Nav_ClearGroundSeeds );
    LUA->SetField( -2, "ClearGroundSeeds" );
    LUA->PushCFunction( Nav_ClearAirSeeds );
    LUA->SetField( -2, "ClearAirSeeds" );
    
    LUA->PushCFunction( Nav_SetupMaxDistance );
    LUA->SetField( -2, "SetupMaxDistance" );
    LUA->PushCFunction( Nav_Generate );
    LUA->SetField( -2, "Generate" );
    LUA->PushCFunction( Nav_FullGeneration );
    LUA->SetField( -2, "FullGeneration" );
    LUA->PushCFunction( Nav_IsGenerated );
    LUA->SetField( -2, "IsGenerated" );
    LUA->PushCFunction( Nav_FindPath );
    LUA->SetField( -2, "FindPath" );
    LUA->PushCFunction( Nav_FindPathImmediate );
    LUA->SetField( -2, "FindPathImmediate" );
    
    LUA->PushCFunction( Nav_FindPathHull );
    LUA->SetField( -2, "FindPathHull" );
    LUA->PushCFunction( Nav_GetHeuristic );
    LUA->SetField( -2, "GetHeuristic" );
    LUA->PushCFunction( Nav_GetStart );
    LUA->SetField( -2, "GetStart" );
    LUA->PushCFunction( Nav_GetEnd );
    LUA->SetField( -2, "GetEnd" );
    LUA->PushCFunction( Nav_SetHeuristic );
    LUA->SetField( -2, "SetHeuristic" );
    
    LUA->PushCFunction( Nav_SetStart );
    LUA->SetField( -2, "SetStart" );
    LUA->PushCFunction( Nav_SetEnd );
    LUA->SetField( -2, "SetEnd" );
    LUA->PushCFunction( Nav_GetNode );
    LUA->SetField( -2, "GetNode" );
    LUA->PushCFunction( Nav_GetClosestNode );
    LUA->SetField( -2, "GetClosestNode" );
    LUA->PushCFunction( Nav_GetNearestNodes );
    LUA->SetField( -2, "GetNearestNodes" );
    
    LUA->PushCFunction( Nav_GetNearestNodes );
    LUA->SetField( -2, "GetNodesInSphere" );
    LUA->PushCFunction( Nav_Save );
    LUA->SetField( -2, "Save" );
    LUA->PushCFunction( Nav_Load );
    LUA->SetField( -2, "Load" );
    LUA->PushCFunction( Nav_GetDiagonal );
    LUA->SetField( -2, "GetDiagonal" );
    LUA->PushCFunction( Nav_SetDiagonal );
    LUA->SetField( -2, "SetDiagonal" );
    
    LUA->PushCFunction( Nav_GetGridSize );
    LUA->SetField( -2, "GetGridSize" );
    LUA->PushCFunction( Nav_SetGridSize );
    LUA->SetField( -2, "SetGridSize" );
    LUA->PushCFunction( Nav_GetMask );
    LUA->SetField( -2, "GetMask" );
    LUA->PushCFunction( Nav_SetMask );
    LUA->SetField( -2, "SetMask" );
    LUA->PushCFunction( Nav_CreateNode );
    LUA->SetField( -2, "CreateNode" );
    LUA->PushCFunction( Nav_RemoveNode );
    LUA->SetField( -2, "RemoveNode" );
    
    LUA->SetField( -2, "__index" );
    LUA->Pop();
    
    LUA->CreateMetaTableType(NODE_NAME, NODE_TYPE);
    LUA->CreateTable();
    
    LUA->PushCFunction(Node__eq);
    LUA->SetField( -2, "__eq" );
    LUA->PushCFunction( Node_GetID );
    LUA->SetField( -2, "GetID" );
    LUA->PushCFunction( Node_GetPosition );
    LUA->SetField( -2, "GetPosition" );
    LUA->PushCFunction( Node_GetPosition );
    LUA->SetField( -2, "GetPos" );
    LUA->PushCFunction( Node_GetNormal );
    LUA->SetField( -2, "GetNormal" );
    LUA->PushCFunction( Node_GetConnections );
    LUA->SetField( -2, "GetConnections" );
    LUA->PushCFunction( Node_GetConnectedNode );
    LUA->SetField( -2, "GetConnectedNode" );
    LUA->PushCFunction( Node_GetScoreF );
    LUA->SetField( -2, "GetScoreF" );
    LUA->PushCFunction( Node_GetScoreG );
    LUA->SetField( -2, "GetScoreG" );
    LUA->PushCFunction( Node_IsConnected );
    LUA->SetField( -2, "IsConnected" );
    LUA->PushCFunction( Node_IsDisabled );
    LUA->SetField( -2, "IsDisabled" );
    LUA->PushCFunction( Node_SetDisabled );
    LUA->SetField( -2, "SetDisabled" );
    LUA->PushCFunction( Node_SetNormal );
    LUA->SetField( -2, "SetNormal" );
    LUA->PushCFunction( Node_SetPosition );
    LUA->SetField( -2, "SetPosition" );
    LUA->PushCFunction( Node_ConnectTo );
    LUA->SetField( -2, "ConnectTo" );
    LUA->PushCFunction( Node_RemoveConnection );
    LUA->SetField( -2, "RemoveConnection" );
    
    LUA->SetField( -2, "__index" );
    LUA->PushCFunction( Node__eq );
    LUA->SetField( -2, "__eq" );
    LUA->Pop();
    
    if( !AddPollHook(state) )
        Msg("gmsv_navigation: Failed to add poll hook\n");

#ifdef IS_SERVERSIDE
	Msg("gmsv_navigation_osx: Loaded\n");
#else
	Msg("gmcl_navigation_osx: Loaded\n");
#endif

#ifdef FILEBUG
	pDebugFile = fopen("garrysmod/filebug.txt", "a+");
	FILEBUG_WRITE("Opened Module\n");
#endif

	return 0;
}

GMOD_MODULE_CLOSE()
{
#ifdef USE_BOOST_THREADS
	Msg("gm_navigation: Aborting threads...\n");
	for(int i=0; i < JobQueue.Size(); i++)
	{
		JobQueue[i]->abort = true;

		/*
		if(JobQueue[i]->thread.joinable())
		{
			JobQueue[i]->thread.join();
		}
		*/
		
		Msg("gm_navigation: Aborting Thread %d\n", i);

#ifdef FILEBUG
		FILEBUG_WRITE("Aborting...\n");
#endif

		while(!JobQueue[i]->finished)// && !JobQueue[i]->thread.timed_join(timeout))
		{
			boost::this_thread::sleep(sleep_time);
			Msg("Waiting...\n");
		}

#ifdef FILEBUG
		FILEBUG_WRITE("Aborted\n");
#endif

		Msg("gm_navigation: Aborted\n");
	}

	Msg("gm_navigation: Done\n");

#else
	if(threadPool != NULL)
	{
		for(int i=0; i < JobQueue.Size(); i++)
		{
			JobQueue[i]->abort = true;
		}
		threadPool->Stop();
		V_DestroyThreadPool(threadPool);
		threadPool = NULL;
	}
#endif

#ifdef FILEBUG
	if(pDebugFile != NULL)
	{
		fputs("Closed Module\n", pDebugFile);
		fclose(pDebugFile);
	}
#endif

	return 0;
}
