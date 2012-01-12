#include <vision/visionNode.h>
#include <pcrctransformation/pcrctransformer.hpp>
#include <pcrctransformation/point2f.hpp>
#include <CameraCalibration/RectifyImage.h>
#include <unicap_cv_bridge.hpp>
#include <FiducialDetector.h>
#include <QRCodeDetector.h>
#include <Crate.h>
#include <ros/ros.h>

#include <vision/CrateEventMsg.h>
#include <vision/error.h>
#include <vision/getCrate.h>
#include <vision/getAllCrates.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <sstream>
#include <map>

using namespace unicap_cv_bridge;
using namespace pcrctransformation;
using namespace std;
using namespace cv;

visionNode::visionNode(int argc, char* argv[]){
		if (argc != 4){
			printUsage(argv[0]);
			exit(1);
		}
		int device_number = atoi(argv[1]);
		int format_number = atoi(argv[2]);
		cam = new unicap_cv_camera(device_number,format_number);
		cam->set_auto_white_balance(true);
		cam->set_exposure(0.017);
		camFrame = Mat(cam->get_img_height(), cam->get_img_width(), cam->get_img_format());

		fidDetector = new FiducialDetector();
		fidDetector->minRad = 10;

		qrDetector = new QRCodeDetector();

		point2f::point2fvector rc;
		rc.push_back(point2f(71, 110));
		rc.push_back(point2f(-54, 113));
		rc.push_back(point2f(-59, -75));
		cordTransformer= new pc_rc_transformer(rc,rc);

		rectifier = new RectifyImage();
		if(!rectifier->initRectify(argv[3], cv::Size( cam->get_img_width(),cam->get_img_height()))){
			cout << "XML not found" << endl;
			exit(2);
		}
		crateMovementThresshold = 5;
		crateTracker = new CrateTracker(10 , crateMovementThresshold);




		crateEventPublisher = node.advertise<vision::CrateEventMsg>("crateEvent", 100);
		ErrorPublisher = node.advertise<vision::error>("visionError", 100);
		getCrateService = node.advertiseService("getCrate", &visionNode::getCrate, this);
		getAllCratesService = node.advertiseService("getAllCrates", &visionNode::getAllCrates, this);
}

bool visionNode::getCrate(vision::getCrate::Request &req,vision::getCrate::Response &res)
{
	exCrate crate;
	bool succeeded = crateTracker->getCrate(req.name, crate);
	if(succeeded){
		res.state = crate.getState();
		vision::CrateMsg msg;
		msg.name = crate.name;
		msg.x =crate.rect().center.x;
		msg.y =crate.rect().center.y;
		msg.angle = crate.rect().angle;
		res.crate = msg;
	} else{
		res.state = exCrate::state_non_existing;
		vision::CrateMsg msg;
		msg.name = "";
		msg.x =0;
		msg.y =0;
		msg.angle = 0;
		res.crate = msg;
	}
	return true;
}

bool visionNode::getAllCrates(vision::getAllCrates::Request &req,vision::getAllCrates::Response &res)
{
	std::vector<exCrate> allCrates = crateTracker->getAllCrates();
	for(std::vector<exCrate>::iterator it = allCrates.begin(); it != allCrates.end(); ++it)
	{
		res.states.push_back(it->getState());
		vision::CrateMsg msg;
		msg.name =it->name;
		msg.x =it->rect().center.x;
		msg.y =it->rect().center.y;
		msg.angle = it->rect().angle;
		res.crates.push_back(msg);
	}
	return true;
}

visionNode::~visionNode(){
	delete cam;
	delete fidDetector;
	delete qrDetector;
	delete cordTransformer;
	delete rectifier;
	delete crateTracker;
}

void visionNode::printUsage(char* invokeName){
	printf("usage for starting capture: %s device_number format_number correction xml\n", invokeName);
}

void visionNode::run(){
	while(ros::ok()){
		cam->get_frame(&camFrame);
		rectifier->rectify(camFrame, rectifiedCamFrame);
		cv::Mat gray;
		cv::cvtColor(rectifiedCamFrame, gray, CV_BGR2GRAY);

		std::vector<cv::Point2f> fiducialPoints;
		fidDetector->detect(gray, fiducialPoints,&rectifiedCamFrame);
		if(fiducialPoints.size() == 3){
			point2f::point2fvector pixelCoordinatesFiducial;
			Crate::order(fiducialPoints);
			for(int n = 0; n <3; n++){
				pixelCoordinatesFiducial.push_back(point2f(fiducialPoints[n].x, fiducialPoints[n].y));
			}
			cordTransformer->set_fiducials_pixel_coordinates(pixelCoordinatesFiducial);
		} else {
			//ROS_WARN("Number of fiducials not correct needed 3 saw: %d", fiducialPoints.size());
		}

		std::vector<Crate> crates;
		qrDetector->detectCrates(gray, crates);
		for(std::vector<Crate>::iterator it=crates.begin(); it!=crates.end(); ++it)
		{
			it->draw(rectifiedCamFrame);


			std::vector<cv::Point2f> points;
			for(int n = 0; n <3; n++){
				point2f result = cordTransformer->to_rc(point2f(it->getPoints()[n].x, it->getPoints()[n].y));
				points.push_back(cv::Point2f(result.x, result.y));
			}

			//transfor coordinates
			it->setPoints(points);

			ROS_INFO("crate pos: %f %f", it->rect().center.x, it->rect().center.y);
		}
		std::vector<CrateEvent> events = crateTracker->update(crates);

		for(std::vector<CrateEvent>::iterator it = events.begin(); it != events.end(); ++it)
		{
			vision::CrateEventMsg msg;
			msg.event = it->type;
			msg.crate.name = it->name;
			msg.crate.x = it->x;
			msg.crate.y = it->y;
			msg.crate.angle = it->angle;

			ROS_INFO(it->toString().c_str());
			crateEventPublisher.publish(msg);
		}
		imshow("image",rectifiedCamFrame);
		waitKey(10);
		ros::spinOnce();
	}
}
