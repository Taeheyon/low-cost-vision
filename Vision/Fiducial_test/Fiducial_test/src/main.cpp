//******************************************************************************
//
//                 Low Cost Vision
//
//******************************************************************************
// Project:        Fiducial_test
// File:           main.cpp
// Description:    Tests the fiducial project
// Author:         Jules Blok
// Notes:          None
//
// License:        GNU GPL v3
//
// This file is part of Fiducial_test.
//
// Fiducial_test is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fiducial_test is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Fiducial_test.  If not, see <http://www.gnu.org/licenses/>.
//******************************************************************************

#include <string>
#include <iostream>
#include <iomanip>
#include <set>
#include <map>
#include <boost/foreach.hpp>
#include <imageMetaData/Types.hpp>
#include <imageMetaData/Tools.hpp>
#include <report/Report.hpp>
#include <report/CategoryOverview.hpp>
#include <report/ReportHistogram.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "FiducialDetector.h"
#include "CrateDetector.h"
#include "QRCodeDetector.h"
#include "Crate.h"


#ifdef __CDT_PARSER__
#define foreach(a, b) for(a : b)
#else
#define foreach(a, b) BOOST_FOREACH(a, b)
#endif

#define MAX_DEVIATION 5
#define MAX_RANGE 5

using namespace std;
using namespace imageMetaData;
using namespace report;

inline float dist(cv::Point2f p1, cv::Point2f p2) {
	float dx = p1.x-p2.x;
	float dy = p1.y-p2.y;
	return sqrt(dx*dx+dy*dy);
}

int main(int argc, char** argv){
        if (argc < 2) {
                cout << "Usage: " << argv[0] << " <xml path>\n";
                return 1;
        }

        cout << setiosflags(ios::left) << setiosflags(ios::fixed);

        // Load the metadata of the images from an XML file
        vector<ImageMD> images = getMetaData(argv[1]);

        // Create containers for storing the result per image.
        unsigned int size = images.size();

        // Deviation results
        vector<double> fidResults(size);
        vector<double> qrResults(size);
        vector<double> calibResults(size);

        // Expected results
        vector<int> fidCounts(size);
        vector<int> qrCounts(size);
        vector<int> calibCounts(size);

        // Detected results
        vector<int> fidSizes(size);
        vector<int> qrSizes(size);
        vector<int> calibSizes(size);

        // Create a container for storing the categories and their results
        CategoriesResults fidCats;
        CategoriesResults qrCats;
        CategoriesResults calibCats;

        // The vision algorithm
        FiducialDetector fidDetector;
        CrateDetector crateDetector;
        QRCodeDetector qrDetector;

        // Loop through all the images
        for(unsigned int i=0; i<images.size(); i++) {
			bool fidSuccess = true;
			bool qrSuccess = true;
			bool calibSuccess = true;
			fidCounts[i] = 0;
			qrCounts[i] = 0;
			calibCounts[i] = 0;

			cv::Mat image = cv::imread(images[i].path, CV_LOAD_IMAGE_GRAYSCALE);
			if(!image.data) {
				cout << "Image " << images[i].path << " not found! Aborting test..." << endl;
				return 1;
			}
			cout << "Testing image " << images[i].name << endl;

			vector<cv::Point2f> points;
			vector<Crate> fidCrates;
			vector<Crate> qrCrates;
			vector<cv::Point2f> calib;
			fidDetector.detect(image, points);
			crateDetector.detect(image, points, fidCrates, &calib);
			qrDetector.detectCrates(image, qrCrates);
			fidSizes[i] = fidCrates.size();
			qrSizes[i] = qrCrates.size();
			calibSizes[i] = calib.size();

			for(std::vector<Crate>::iterator it=fidCrates.begin(); it!=fidCrates.end(); ++it) it->draw(image);
			for(std::vector<Crate>::iterator it=qrCrates.begin(); it!=qrCrates.end(); ++it) it->draw(image);
			//cv::imshow("Image", image);
			//cv::waitKey();

			foreach(Properties object, images[i].objects) {
				if(object["type"] == "QR code") {
					qrCounts[i]++;
					if(!qrCrates.empty()) {
						cv::Point2f ptTarget((double)object["x"], (double)object["y"]);
						float lastDistance = dist(qrCrates[0].rect().center, ptTarget);
						Crate result = qrCrates[0];

						// Get the crate closest to the target
						foreach(Crate crate, qrCrates) {
							float d = dist(crate.rect().center,ptTarget);
							if(d < lastDistance) {
								result = crate;
								lastDistance = d;
							}
						}

						std::vector<cv::Point2f> points = result.getPoints();
						vector<cv::Point2f> targetPoints;
						targetPoints.push_back(cv::Point2f((double)object["marker1.x"], (double)object["marker1.y"]));
						targetPoints.push_back(cv::Point2f((double)object["marker2.x"], (double)object["marker2.y"]));
						targetPoints.push_back(cv::Point2f((double)object["marker3.x"], (double)object["marker3.y"]));
						float totalDistance = 0;

						for(unsigned int j=0; j<3; j++) {
							float d = dist(points[j], targetPoints[j]);
							if(d > MAX_DEVIATION) {
								qrSuccess = false;
							}
							totalDistance += d;
						}

						qrResults[i] += totalDistance / 3.0;
					}
					else {
						qrSuccess = false;
					}
				}
				else if(object["type"] == "Crate") {
					fidCounts[i]++;
					if(!fidCrates.empty()) {
						cv::Point2f ptTarget((double)object["x"], (double)object["y"]);
						float lastDistance = dist(fidCrates[0].rect().center, ptTarget);
						Crate result = fidCrates[0];

						// Get the crate closest to the target
						foreach(Crate crate, fidCrates) {
							float d = dist(crate.rect().center,ptTarget);
							if(d < lastDistance) {
								result = crate;
								lastDistance = d;
							}
						}

						std::vector<cv::Point2f> points = result.getPoints();
						vector<cv::Point2f> targetPoints;
						targetPoints.push_back(cv::Point2f((double)object["fid1.x"], (double)object["fid1.y"]));
						targetPoints.push_back(cv::Point2f((double)object["fid2.x"], (double)object["fid2.y"]));
						targetPoints.push_back(cv::Point2f((double)object["fid3.x"], (double)object["fid3.y"]));
						float totalDistance = 0;

						for(unsigned int j=0; j<3; j++) {
							float d = dist(points[j], targetPoints[j]);
							if(d > MAX_DEVIATION) {
								fidSuccess = false;
							}
							totalDistance += d;
						}

						fidResults[i] += totalDistance / 3.0;
					}
					else {
						fidSuccess = false;
					}
				}
				else if(object["type"] == "Marker") {
					calibCounts[i]++;
					if(!calib.empty()) {
						cv::Point2f ptTarget((double)object["x"], (double)object["y"]);
						float lastDistance = dist(calib[0], ptTarget);
						cv::Point2f result = calib[0];

						// Get the crate closest to the target
						foreach(cv::Point2f point, calib) {
							float d = dist(point,ptTarget);
							if(d < lastDistance) {
								result = point;
								lastDistance = d;
							}
						}

						calibResults[i] += dist(result, ptTarget);
					}
					else {
						calibSuccess = false;
					}
				}
			}

			if(fidCounts[i] != 0) fidResults[i] /= fidCounts[i];
			else fidResults[i] = -1.0;
			// Store the results in the categories container
			foreach(Category c, images[i].categories) {
				if(fidSuccess){
					// Increment number of correct images in category
					fidCats[c.first][c.second].first++;
				}
				// Increment total number images in category
				fidCats[c.first][c.second].second++;
			}

			if(qrCounts[i] != 0) qrResults[i] /= qrCounts[i];
			else qrResults[i] = -1.0;
			// Store the results in the categories container
			foreach(Category c, images[i].categories) {
				if(qrSuccess){
					// Increment number of correct images in category
					qrCats[c.first][c.second].first++;
				}
				// Increment total number images in category
				qrCats[c.first][c.second].second++;
			}

			if(calibCounts[i] != 0) calibResults[i] /= calibCounts[i];
			else calibResults[i] = -1.0;

			if(calibCounts[i] < 3) calibSuccess = false;
			// Store the results in the categories container
			foreach(Category c, images[i].categories) {
				if(calibSuccess){
					// Increment number of correct images in category
					calibCats[c.first][c.second].first++;
				}
				// Increment total number images in category
				calibCats[c.first][c.second].second++;
			}
        }

		//========================================================================
		// Create the report
		//========================================================================
        cout << "Writing report..." << endl;
        Report r("Fiducial report");

        /* Fiducial */

        // Put the result of each image in a ReportList
        ReportList* fidList = new ReportList("Fiducial results", 2, STRING, DOUBLE);
        for(unsigned int i=0; i<images.size(); i++) {
        	fidList->appendRow(images[i].name, fidResults[i]);
        }
        fidList->setColumnNames("Image", "Mean deviation");
        r.addField(fidList);

        // Put the count of each image in a ReportList
        ReportList* fidCount = new ReportList("Detected fiducials", 3, STRING, INT, INT);
        for(unsigned int i=0; i<images.size(); i++) {
        	fidCount->appendRow(images[i].name, fidSizes[i], fidCounts[i]);
        }
        fidCount->setColumnNames("Image", "Detected", "Tagged");
        r.addField(fidCount);

        // Create a histogram of the results
        ReportHistogram* fidHis = new ReportHistogram("Fiducial histogram", fidResults, MAX_RANGE*2, MAX_RANGE);
        fidHis->setColumnNames("Deviation range", "Images with deviation");
        r.addField(fidHis);

        // Add all categories and their results to the main report
        foreach(CategoryResults cr, fidCats) {
                CategoryOverview* cat = new CategoryOverview(cr);
                cat->setColumnNames("Category", "Correct images", "Percentage correct");
                r.addField(cat);
        }

        /* QR Code */

        // Put the result of each image in a ReportList
		ReportList* qrList = new ReportList("QRCode results", 2, STRING, DOUBLE);
		for(unsigned int i=0; i<images.size(); i++) {
			qrList->appendRow(images[i].name, fidResults[i]);
		}
		qrList->setColumnNames("Image", "Mean deviation");
		r.addField(qrList);

		// Put the count of each image in a ReportList
		ReportList* qrCount = new ReportList("Detected QRCodes", 3, STRING, INT, INT);
		for(unsigned int i=0; i<images.size(); i++) {
			qrCount->appendRow(images[i].name, qrSizes[i], qrCounts[i]);
		}
		qrCount->setColumnNames("Image", "Detected", "Tagged");
		r.addField(qrCount);

        // Create a histogram of the results
        ReportHistogram* qrHis = new ReportHistogram("QRCode histogram", qrResults, MAX_RANGE*2, MAX_RANGE);
        qrHis->setColumnNames("Deviation range", "Images with deviation");
        r.addField(qrHis);

        // Add all categories and their results to the main report
        foreach(CategoryResults cr, qrCats) {
                CategoryOverview* cat = new CategoryOverview(cr);
                cat->setColumnNames("Category", "Correct images", "Percentage correct");
                r.addField(cat);
        }

        /* Calibration marker */

        // Put the result of each image in a ReportList
		ReportList* calibList = new ReportList("Calibration results", 2, STRING, DOUBLE);
		for(unsigned int i=0; i<images.size(); i++) {
			calibList->appendRow(images[i].name, calibResults[i]);
		}
		calibList->setColumnNames("Image", "Mean deviation");
		r.addField(calibList);

		// Put the count of each image in a ReportList
		ReportList* calibCount = new ReportList("Detected markers", 3, STRING, INT, INT);
		for(unsigned int i=0; i<images.size(); i++) {
			calibCount->appendRow(images[i].name, calibSizes[i], calibCounts[i]);
		}
		calibCount->setColumnNames("Image", "Detected", "Tagged");
		r.addField(calibCount);

        // Create a histogram of the results
        ReportHistogram* calibHis = new ReportHistogram("Calibration histogram", calibResults, MAX_RANGE*2, MAX_RANGE);
        calibHis->setColumnNames("Deviation range", "Images with deviation");
        r.addField(calibHis);

        // Add all categories and their results to the main report
        foreach(CategoryResults cr, calibCats) {
                CategoryOverview* cat = new CategoryOverview(cr);
                cat->setColumnNames("Category", "Correct images", "Percentage correct");
                r.addField(cat);
        }

        r.saveHTML("results.html");

        cout << "Fiducial testbench done" << endl;

        return 0;
}

