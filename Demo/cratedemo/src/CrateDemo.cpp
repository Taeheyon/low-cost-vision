//******************************************************************************
//
//                 Low Cost Vision
//
//******************************************************************************
// Project:        cratedemo
// File:           CrateDemo.cpp
// Description:    Framework for a demo with crates.
// Author:         Lukas Vermond
// Notes:
//
// License: newBSD 
//  
// Copyright © 2012, HU University of Applied Sciences Utrecht. 
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// - Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// - Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
// - Neither the name of the HU University of Applied Sciences Utrecht nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE HU UNIVERSITY OF APPLIED SCIENCES UTRECHT
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//******************************************************************************

#include <cratedemo/CrateDemo.hpp>

#include <cassert>
#include <iostream>

#include <ros/ros.h>
#include <vision/getAllCrates.h>
#include <deltarobotnode/gripper.h>
#include <cratedemo/GridCrate4x4MiniBall.hpp>
#include <cratedemo/MotionWrapper.hpp>
#include <cratedemo/CrateExceptions.hpp>

namespace cratedemo
{

void CrateDemo::staticActionThreadFunc(CrateDemo* obj)
{
	obj->actionThreadFunc();
}

CrateDemo::CrateDemo(
	ros::NodeHandle& hNode,
	const std::string& deltaGrip,
	const std::string& deltaStop,
	const std::string& deltaMotion,
	const std::string& checkMotion,
	const std::string& deltaError,
	const std::string& crateRefresh,
	const std::string& getCrate,
	const std::string& visionEvents,
	const std::string& visionError,
	CrateContentMap& crateContentMap) :
		gripperClient( hNode.serviceClient<deltarobotnode::gripper>(deltaGrip) ),
		motionClient( hNode.serviceClient<deltarobotnode::motionSrv>(deltaMotion) ),
		checkClient( hNode.serviceClient<deltarobotnode::motionSrv>(checkMotion) ),
		stopClient(hNode.serviceClient<deltarobotnode::stop>(deltaStop)),
		deltaErrorSub(hNode.subscribe(deltaError, 1000, &CrateDemo::deltaErrorCb, this)),
		crateRefreshClient(hNode.serviceClient<vision::getAllCrates>(crateRefresh)),
		getCrateClient(hNode.serviceClient<vision::getCrate>(getCrate)),
		crateEventSub(hNode.subscribe(visionEvents, 1000, &CrateDemo::crateEventCb, this)),
		visionErrorSub(hNode.subscribe(visionError, 1000, &CrateDemo::visionErrorCb, this)),
		crateContentMap(crateContentMap),
		threadRunning(true) {
	actionThread = new boost::thread(staticActionThreadFunc, this);
}

Crate* CrateDemo::waitForCrate(const std::string& name)
{
	CrateMap::iterator it = crates.find(name);
	while(it == crates.end() || it->second->moving)
	{
		crateMapMutex.unlock();
		boost::unique_lock<boost::mutex> lock(waitMutex);
		waitCondition.wait(lock);
		crateMapMutex.lock();
		it = crates.find(name);
	}

	return it->second;
}

datatypes::point3f CrateDemo::getCrateContentGripLocation(const Crate& crate, size_t index)
{
	try
	{
		return crate.getContentGripLocation(index);
	}
	catch(LocationIsEmptyException& ex)
	{
		std::cerr << "index " << index << " in crate " << crate.getName() << " is empty" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void CrateDemo::actionThreadFunc(void)
{
	try
	{
		while(threadRunning)
		{
			actionQueueMutex.lock();
			if(!actionQueue.empty())
			{
				//pop action
				MoveAction action = actionQueue.front();
				actionQueue.pop();
				actionQueueMutex.unlock();

				//wait for source crate
//				CrateMap::iterator itFrom = crates.find(action.getStrFrom());
//				while(itFrom == crates.end() || itFrom->second->moving)
//				{
//					crateMapMutex.unlock();
//					boost::unique_lock<boost::mutex> lock(waitMutex);
//					waitCondition.wait(lock);
//					crateMapMutex.lock();
//					itFrom = crates.find(action.getStrFrom());
//				}
//				try {
//					posFrom = itFrom->second->getContentGripLocation(action.getIndexFrom());
//				}
//				catch(LocationIsEmptyException& ex) {
//					std::cerr << "location is empty, location=" << action.getIndexFrom() << std::endl;
//					exit(EXIT_FAILURE);
//				}

				Crate* crateFrom;
				datatypes::point3f posFrom;

				//move to source
				MotionWrapper motionToSource;
				for(;;)
				{
					crateMapMutex.lock();
					//wait for source crate
					crateFrom = waitForCrate(action.getStrFrom());
					//get source location
					posFrom = getCrateContentGripLocation(*crateFrom, action.getIndexFrom());

					motionToSource = MotionWrapper();
					motionToSource.addMotion(datatypes::point3f(posFrom.x, posFrom.y, SAFE_HEIGHT), 123);
					motionToSource.addMotion(posFrom, 123);

					//if object in crate is not reachable, then wait for movement and check again
					if (!motionToSource.callService(motionClient))
					{
						crateMapMutex.unlock();
						ROS_INFO("Cannot reach source location. Waiting till robot can reach it.");
						boost::unique_lock<boost::mutex> lock(waitMutex);
						waitCondition.wait(lock);

					}
					else
					{
						break;
					}
				}

				//remove content from source crate
				CrateContent* content = crateFrom->get(action.getIndexFrom());
				crateFrom->remove(action.getIndexFrom());
				crateMapMutex.unlock();

				//grip
				deltarobotnode::gripper grip;
				grip.request.enabled = true;
				gripperClient.call(grip);

				boost::this_thread::sleep(boost::posix_time::milliseconds(500));

				//move up
				MotionWrapper motionToSourceUp;
				motionToSourceUp.addMotion(datatypes::point3f(posFrom.x, posFrom.y, SAFE_HEIGHT), 36);
				motionToSourceUp.callService(motionClient);

				Crate* crateTo;
				datatypes::point3f posTo;

				//move to destination
				MotionWrapper motionToDestination;
				for(;;)
				{
					crateMapMutex.lock();
					//wait for source crate
					crateTo = waitForCrate(action.getStrTo());
					//get source location
					posTo = crateTo->getContainerLocation(action.getIndexTo()) + content->getGripPoint();

					motionToDestination = MotionWrapper();
					motionToDestination.addMotion(datatypes::point3f(posTo.x, posTo.y, SAFE_HEIGHT), 123);
					motionToDestination.addMotion(posTo, 123);

					//if drop location in crate is not reachable, then wait for movement and check again
					if (!motionToDestination.callService(motionClient))
					{
						crateMapMutex.unlock();
						ROS_INFO("Cannot reach destination location. Waiting till robot can reach it.");
						boost::unique_lock<boost::mutex> lock(waitMutex);
						waitCondition.wait(lock);
					}
					else
					{
						break;
					}
				}

				try
				{
					posTo = crateTo->getContainerLocation(action.getIndexTo()) + content->getGripPoint();
					crateTo->put(action.getIndexTo(), content);
					crateMapMutex.unlock();
				}
				catch(LocationIsFullException& ex)
				{
					std::cerr << "location is full, location=" << action.getIndexFrom() << std::endl;
					exit(EXIT_FAILURE);
				}

				//crateMapMutex.unlock();

				//wait for destination crate
//				CrateMap::iterator itTo = crates.find(action.getStrTo());
//				while(itTo == crates.end() || itTo->second->moving)
//				{
//					crateMapMutex.unlock();
//					boost::unique_lock<boost::mutex> lock(waitMutex);
//					waitCondition.wait(lock);
//					crateMapMutex.lock();
//					itTo = crates.find(action.getStrTo());
//				}

				//get destination crate location
//				datatypes::point3f posTo;

				//put content in destination crate
//				try{
//					posTo = itTo->second->getContainerLocation(action.getIndexTo()) + content->getGripPoint();
//					itTo->second->put(action.getIndexTo(), content);
//					crateMapMutex.unlock();
//				}
//				catch(LocationIsFullException& ex){
//					std::cerr << "location is full, location=" << action.getIndexFrom() << std::endl;
//					exit(EXIT_FAILURE);
//				}

				//move to destination, down
//				MotionWrapper motionToDest;
//				motionToDest.addMotion(datatypes::point3f(posTo.x, posTo.y, SAFE_HEIGHT), 123);
//				motionToDest.addMotion(datatypes::point3f(posTo.x, posTo.y, posTo.z + 4), 65); //TODO: remove temporary solution 'z + 4'
//				while(!motionToDest.callService(motionClient))
//				{
//					boost::unique_lock<boost::mutex> lock(waitMutex);
//					waitCondition.wait(lock);
//				}

				//drop
				grip.request.enabled = false;
				gripperClient.call(grip);

				//wait for the vacuum to subside
				boost::this_thread::sleep(boost::posix_time::milliseconds(200));

				//MotionWrapper LooseBall;
				//LooseBall.addMotion(datatypes::point3f(posTo.x + 2, posTo.y + 2, posTo.z + 6), 123);
				//LooseBall.callService(motionClient);

				//move up
				MotionWrapper motionToDestUp;
				motionToDestUp.addMotion(datatypes::point3f(posTo.x, posTo.y, SAFE_HEIGHT), 123);
				motionToDestUp.callService(motionClient);
			}
			else //empty
			{
				actionQueueMutex.unlock();

				idleMutex.lock();
				idle = true;
				idleMutex.unlock();
				idleCondition.notify_all();

				boost::unique_lock<boost::mutex> lock(idleMutex);
				while(idle) { idleCondition.wait(lock); }
			}
		}
	}
	catch(boost::thread_interrupted& ex) {}
	catch(std::exception& ex){
		std::cerr << "exception of type " << typeid(ex).name() << " occurred in action thread. what(): " << ex.what() << std::endl;
		exit(EXIT_FAILURE);
	}
}

void CrateDemo::deltaErrorCb(const deltarobotnode::error::ConstPtr& msg)
{
	ROS_ERROR("Delta node error[%i]:\t%s",msg->errorType,msg->errorMsg.c_str());
	onDeltaError(msg->errorType, msg->errorMsg);
}

void CrateDemo::visionErrorCb(const vision::error::ConstPtr& msg)
{
	ROS_ERROR("Vision node error[%i]:\t%s", msg->errorType, msg->errorMsg.c_str());
	onVisionError(msg->errorType, msg->errorMsg);
}

CrateDemo::~CrateDemo()
{
	threadRunning = false;
	actionThread->interrupt();
	actionThread->join();
	delete actionThread;
}

void CrateDemo::crateEventCb(const vision::CrateEventMsg::ConstPtr& msg)
{
	enum
	{
		IN=1,
		OUT,
		MOVING,
		MOVED
	};

	switch(msg->event)
	{
	case IN: handleNewCrate(msg->crate); break;
	case MOVED: handleCrateMoved(msg->crate); break;
	case MOVING: handleCrateMoving(msg->crate); break;
	case OUT: handleCrateRemoved(msg->crate); break;
	default: assert(0 && "unknown crate event"); break;
	}

	waitCondition.notify_all();
}

void CrateDemo::handleNewCrate(const vision::CrateMsg& msg)
{
	crateMapMutex.lock();

	bool moving = false;
	CrateContentMap::iterator it = crateContentMap.find(msg.name);
	if(it == crateContentMap.end()){
		ROS_ERROR("NewCrate: Crate content unknown, crate ignored: %s", msg.name.c_str());
		crateMapMutex.unlock();
		return;
	}

	Crate* crate = new GridCrate4x4MiniBall(msg.name, it->second,datatypes::point2f(msg.x,msg.y),msg.angle, moving);
	crates.insert(std::pair<std::string, Crate*>(crate->getName(), crate));

	onNewCrate(*crate);
	crateMapMutex.unlock();
}

void CrateDemo::handleCrateRemoved(const vision::CrateMsg& msg)
{
	crateMapMutex.lock();
	std::map<std::string, Crate*>::iterator result = crates.find(msg.name);

	if(result == crates.end()){
		ROS_WARN("CrateRemoved: Crate didn't exist, remove action ignored: %s", msg.name.c_str());
		crateMapMutex.unlock();
		return;
	}

	onCrateRemoved(*(result->second));
	delete result->second;
	crates.erase(result);

	crateMapMutex.unlock();
}

void CrateDemo::handleCrateMoved(const vision::CrateMsg& msg)
{
	crateMapMutex.lock();
	CrateMap::iterator res = crates.find(msg.name);

	if(res == crates.end()){
		ROS_WARN("CrateMoved: Crate didn't exist, move action ignored: %s", msg.name.c_str());
		crateMapMutex.unlock();
		return;
	}
/*
	if(res != crates.end()){
		vision::getAllCrates::Response allCrates;
		vision::getAllCrates::Request req;
		crateRefreshClient.call(req, allCrates);
		unsigned int i = 0;
		for(; i < allCrates.crates.size(); i++){
			if(allCrates.crates.at(i).name == msg.name){
				handleNewCrate(allCrates.crates.at(i));
				ROS_WARN("CrateMoved: Crate didn't exist, crate added: %s", msg.name.c_str());
				break;
			}
		}
		if(i >= allCrates.crates.size()){
			ROS_ERROR("CrateMoved: Crate didn't exist, crate ignored: %s", msg.name.c_str());
			return;
		}else{
			res = crates.find(msg.name);
			onCrateMove(*(res->second));
			return;
		}
	}
*/
	Crate* c = res->second;
	c->position = datatypes::point2f(msg.x,msg.y);
	c->angle = msg.angle;
	c->moving = false;
	onCrateMove(*c);
	crateMapMutex.unlock();
}

void CrateDemo::handleCrateMoving(const vision::CrateMsg& msg)
{

	crateMapMutex.lock();
	CrateMap::iterator res = crates.find(msg.name);

	if(res == crates.end()){
		ROS_WARN("CrateMoving: Crate didn't exist, moving action ignored: %s", msg.name.c_str());
		crateMapMutex.unlock();
		return;
	}
/*
	if(res != crates.end()){
		vision::getAllCrates::Response allCrates;
		vision::getAllCrates::Request req;
		crateRefreshClient.call(req, allCrates);
		unsigned int i = 0;
		for(; i < allCrates.crates.size(); i++){
			if(allCrates.crates.at(i).name == msg.name){
				handleNewCrate(allCrates.crates.at(i));
				ROS_WARN("CrateMoving: Crate didn't exist, crate added: %s", msg.name.c_str());
				break;
			}
		}
		if(i >= allCrates.crates.size()){
			ROS_ERROR("CrateMoving: Crate didn't exist, crate ignored: %s", msg.name.c_str());
			return;
		}else{
			res = crates.find(msg.name);
		}
	}
*/

	Crate* c = res->second;
	c->moving = true;
	crateMapMutex.unlock();
}

void CrateDemo::getAllCrates(void){
	vision::getAllCrates allCrates;
	crateRefreshClient.call(allCrates);


	for(unsigned int i = 0; i < allCrates.response.crates.size(); i++){
		crateMapMutex.lock();
		CrateMap::iterator it = crates.find(allCrates.response.crates.at(i).name);
		if(it == crates.end()){
			handleNewCrate(allCrates.response.crates.at(i));
		}else{
			it->second->position.x = allCrates.response.crates.at(i).x;
			it->second->position.y = allCrates.response.crates.at(i).y;
			it->second->angle = allCrates.response.crates.at(i).angle;
		}
		crateMapMutex.unlock();
	}
}

void CrateDemo::moveObject(Crate& crateFrom, size_t indexFrom ,Crate& crateTo, size_t indexTo ){
	actionQueueMutex.lock();
	actionQueue.push(MoveAction(crateFrom.getName(), indexFrom, crateTo.getName(), indexTo));
	actionQueueMutex.unlock();
	idleMutex.lock();
	idle = false;
	idleMutex.unlock();
	idleCondition.notify_all();
}

/*
//DEBUG
static void printMove(const deltarobotnode::motion& move)
{
	std::cout << "printMove invoked" << std::endl;
	if(move.request.x.size() == move.request.y.size() && move.request.x.size() == move.request.z.size() && move.request.x.size() == move.request.speed.size())
	{
		for(size_t i = 0; i < move.request.x.size(); i++)
		{
			std::cout << "i=" << i << " x=" << move.request.x.at(i) << " y=" << move.request.y.at(i) << " z=" << move.request.z.at(i) << " speed=" << move.request.speed.at(i) << std::endl;
		}
	}
	else
	{
		std::cout << "vectors not of same size!" << std::endl;
	}
	std::cout << "printMove returned" << std::endl;
}
*/

//struct MotionCtor
//{
//	deltarobotnode::motion motions;
//
//	void addMotion(const datatypes::point3f& p, float speed)
//	{
//		motions.request.x.push_back(p.x );
//		motions.request.y.push_back(p.y );
//		motions.request.z.push_back(p.z );
//		motions.request.speed.push_back(speed);
//	}
//
//	bool move(ros::ServiceClient& client)
//	{
//		client.call(motions);
//		return motions.response.succeeded;
//	}
//};

/*void CrateDemo::drawCrateCorners(Crate& crate) {
	ROS_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-drawCrateCorners-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
	const int speed = 150;
	//MotionCtor ctor;
	MotionWrapper motionw;

	datatypes::point2f pointLT = datatypes::point2f(-(crate.size.width/2),-(crate.size.depth/2));
	pointLT = pointLT.rotate(crate.angle);
	pointLT += crate.position;
	motionw.addMotion(datatypes::point3lf(pointLT.x,pointLT.y,SAFE_HEIGHT),speed);
	motionw.addMotion(datatypes::point3lf(pointLT.x,pointLT.y,TABLE_HEIGHT + 15),speed);
	motionw.addMotion(datatypes::point3lf(pointLT.x,pointLT.y,SAFE_HEIGHT),speed);

	datatypes::point2f pointRT = datatypes::point2f((crate.size.width/2),-(crate.size.depth/2));
	pointRT = pointRT.rotate(crate.angle);
	pointRT += crate.position;
	motionw.addMotion(datatypes::point3lf(pointRT.x,pointRT.y,SAFE_HEIGHT),speed);
	motionw.addMotion(datatypes::point3lf(pointRT.x,pointRT.y,TABLE_HEIGHT + 15),speed);
	motionw.addMotion(datatypes::point3lf(pointRT.x,pointRT.y,SAFE_HEIGHT),speed);

	datatypes::point2f pointLB = datatypes::point2f(-(crate.size.width/2),(crate.size.depth/2));
	pointLB = pointLB.rotate(crate.angle);
	pointLB += crate.position;
	motionw.addMotion(datatypes::point3lf(pointLB.x,pointLB.y,SAFE_HEIGHT),speed);
	motionw.addMotion(datatypes::point3lf(pointLB.x,pointLB.y,TABLE_HEIGHT + 15),speed);
	motionw.addMotion(datatypes::point3lf(pointLB.x,pointLB.y,SAFE_HEIGHT),speed);

	datatypes::point2f pointRB = datatypes::point2f((crate.size.width/2),(crate.size.depth/2));
	pointRB = pointRB.rotate(crate.angle);
	pointRB += crate.position;
	motionw.addMotion(datatypes::point3lf(pointRB.x,pointRB.y,SAFE_HEIGHT),speed);
	motionw.addMotion(datatypes::point3lf(pointRB.x,pointRB.y,TABLE_HEIGHT + 15),speed);
	motionw.addMotion(datatypes::point3lf(pointRB.x,pointRB.y,SAFE_HEIGHT),speed);

	motionw.print();

	if(!motionw.callService(motionClient))
	{
		ROS_WARN("Can't touch this!");
	}

	ROS_INFO("*-*-*-*-*-*-*-*-*-*-*-*-*-*-drawCrateCorners-*-*-*-*-*-*-*-*-*-*-*-*-*-*");
}*/
//void CrateDemo::moveObject(Crate& crateFrom, size_t indexFrom ,Crate& crateTo, size_t indexTo ){
//	//get positions
//	datatypes::point3f posFrom = crateFrom.getContentGripLocation(indexFrom);
//	datatypes::point3f posTo = crateTo.getContainerLocation(indexTo) + crateFrom.get(indexFrom)->getGripPoint();
//	const double speed = 350;
//
//	//update crate database
//	CrateContent* c = crateFrom.get(indexFrom);
//	crateTo.put(indexTo, c);
//	crateFrom.remove(indexFrom);
//
//	//move to source, down
//	MotionWrapper motion1;
//	motion1.addMotion(datatypes::point3f(posFrom.x, posFrom.y, SAFE_HEIGHT), speed);
//	motion1.addMotion(posFrom, speed);
//	motion1.callService(motionClient);
//
//	//enable gripper
////	deltarobotnode::gripper grip;
////	grip.request.enabled = true;
////	gripperClient.call(grip);
//
//	//move up, dest, down
//	MotionWrapper move2;
//	move2.addMotion(datatypes::point3f(posFrom.x, posFrom.y, SAFE_HEIGHT), speed);
//	move2.addMotion(datatypes::point3f(posTo.x, posTo.y, SAFE_HEIGHT), speed);
//	move2.addMotion(posTo, speed);
//	move2.callService(motionClient);
//
//	//Drop object
////	grip.request.enabled = false;
////	gripperClient.call(grip);
//
//	//loose ball //TODO: this is temporary, delete it
//	MotionWrapper move4;
//	move4.addMotion(datatypes::point3f(posTo.x + 3, posTo.y + 3, posTo.z + 3), speed);
//	move4.callService(motionClient);
//
//	//move up
//	MotionWrapper move3;
//	move3.addMotion(datatypes::point3f(posTo.x, posTo.y, SAFE_HEIGHT), speed);
//	move3.callService(motionClient);
//}
}
