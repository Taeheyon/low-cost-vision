
//******************************************************************************
//
//                 Low Cost Vision
//
//******************************************************************************
// Project:        CrateGUI
// File:           CrateAppImplEvents.cpp
// Description:    The implementation of the events generated in the GUI
// Author:         Glenn Meerstra
// Notes:          ...
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
#include "CrateGUI/CrateAppImpl.h"
#include "CrateGUI/CrateApp.h"

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <iostream>
#include <iomanip>
#include <string>

#ifdef __CDT_PARSER__
#define foreach(a, b) for(a : b)
#else
#define foreach(a, b) BOOST_FOREACH(a, b)
#endif

void CrateAppImpl::OnSizeChange(wxSizeEvent& event){

        MessageLabel->SetLabel(
                        wxT("The values of the box and rotation line are reset"));

        wxSize imageMaxSize = this->GetSize();
        imageMaxSize.SetWidth(
                        imageMaxSize.GetWidth() - bSizer121->GetSize().GetWidth());
        imageMaxSize.SetHeight(
                        imageMaxSize.GetHeight() - MessageLabel->GetSize().GetHeight()
                                        - 20);

        double heightScale, widthScale;
        if (!zoom) {
                heightScale = (double) image.GetHeight()
                                / (double) imageMaxSize.GetHeight();
                widthScale = (double) image.GetWidth()
                                / (double) imageMaxSize.GetWidth();
        } else {
                heightScale = (double) zoomImage.GetHeight()
                                / (double) imageMaxSize.GetHeight();
                widthScale = (double) zoomImage.GetWidth()
                                / (double) imageMaxSize.GetWidth();
        }

        if(!zoom){
                if(widthScale < heightScale){
                        Scale = heightScale;
                }else{
                        Scale = widthScale;
                }
        }else{
                if(widthScale < heightScale){
                        LargestScale = heightScale;
                }else{
                        LargestScale = widthScale;
                }
        }

        QRCorner = wxPoint(0, 0);
        OppositeCorner = wxPoint(0, 0);

       //zoomX = 0;
       // zoomY = 0;
        //zoomWidth = 0;
       // zoomHeight = 0;

        CrateLineRDB->SetValue(true);
        UpdateImageField();

        this->Layout();
}

void CrateAppImpl::OnObjectType( wxCommandEvent& event ){
	CrateLineRDB->SetValue(true);
	if(ObjectTypeCombo->GetValue() == wxT("QR code")){
		QRCodeCornerRDB->SetLabel(wxT("BR point"));
		OppositeCornerRDB->SetLabel(wxT("TL point"));

	}else if(ObjectTypeCombo->GetValue() == wxT("Crate")){
		QRCodeCornerRDB->SetLabel(wxT("QR code"));
		OppositeCornerRDB->SetLabel(wxT("Opposite"));

	}else if(ObjectTypeCombo->GetValue() == wxT("Marker")){
		QRCodeCornerRDB->SetLabel(wxT("Center"));
		OppositeCornerRDB->SetLabel(wxT("Edge"));

	}
}

void CrateAppImpl::OnNextImage( wxMouseEvent& event ){

        zoomX = 0;
        zoomY = 0;
        zoomWidth = 0;
        zoomHeight = 0;

        QRCorner = wxPoint(0, 0);
        OppositeCorner = wxPoint(0, 0);

        zoom = false;

        ZoomButton->Enable(true);
        ZoomBox_radioBtn->Enable(true);
        OriginalImageButton->Enable(false);

        NextImage();
}

void CrateAppImpl::OnLeaveImageField(wxMouseEvent& event){
        mousePressedInImageField = false;
        drawCrateAttributes();
}

void CrateAppImpl::OnLeftMousePressed(wxMouseEvent& event){
        mousePressedInImageField = true;
        std::stringstream coordinate;
        coordinate << std::fixed << std::setprecision(2);

        if(CrateLineRDB->GetValue() || QRCodeCornerRDB->GetValue()){
                QRCorner = event.GetPosition();
                if(!zoom){
                        coordinate << "(" << (QRCorner.x * Scale) << ", " << (QRCorner.y * Scale) << ")";
                }else{
                        coordinate << "(" << (QRCorner.x * Scale + zoomX) << ", " << (QRCorner.y * Scale + zoomY) << ")";
                }
                QRCodeCornerLabel->SetLabel(wxString(coordinate.str().c_str(), wxConvLocal));
        }else if(OppositeCornerRDB->GetValue()){
                OppositeCorner = event.GetPosition();
                if(!zoom){
                        coordinate << "(" << (OppositeCorner.x * Scale) << ", " << (OppositeCorner.y * Scale) << ")";
                }else{
                        coordinate << "(" << (OppositeCorner.x * Scale + zoomX) << ", " << (OppositeCorner.y * Scale + zoomY) << ")";
                }
                OppositeCornerLabel->SetLabel(wxString(coordinate.str().c_str(), wxConvLocal));
        }else if(ZoomBox_radioBtn->GetValue()){
                zoomX = event.GetX();
                zoomY = event.GetY();
                zoomWidth = 0;
                zoomHeight = 0;
        }

        calculateFiducialPoints();
        drawCrateAttributes();
}

void CrateAppImpl::OnLeftMouseRelease(wxMouseEvent& event){
        mousePressedInImageField = false;
        std::stringstream coordinate;
        coordinate << std::fixed << std::setprecision(2);

        if(QRCodeCornerRDB->GetValue()){
                QRCorner = event.GetPosition();
                if(!zoom){
                        coordinate << "(" << (QRCorner.x * Scale) << ", " << (QRCorner.y * Scale) << ")";
                }else{
                        coordinate << "(" << (QRCorner.x * Scale + zoomX) << ", " << (QRCorner.y * Scale + zoomY) << ")";
                }
                QRCodeCornerLabel->SetLabel(wxString(coordinate.str().c_str(), wxConvLocal));
        }else if(CrateLineRDB->GetValue() || OppositeCornerRDB->GetValue()){
                OppositeCorner = event.GetPosition();
                if(!zoom){
                        coordinate << "(" << (OppositeCorner.x * Scale) << ", " << (OppositeCorner.y * Scale) << ")";
                }else{
                        coordinate << "(" << (OppositeCorner.x * Scale + zoomX) << ", " << (OppositeCorner.y * Scale + zoomY) << ")";
                }
                OppositeCornerLabel->SetLabel(wxString(coordinate.str().c_str(), wxConvLocal));
        }else if(ZoomBox_radioBtn->GetValue()){
                zoomWidth = event.GetX() - zoomX;
                zoomHeight = event.GetY() - zoomY;

                if(zoomWidth < 0) {
                        zoomX += zoomWidth;
                        zoomWidth = abs(zoomWidth);
                }

                if(zoomHeight < 0) {
                        zoomY += zoomHeight;
                        zoomHeight = abs(zoomHeight);
                }
        }

        calculateFiducialPoints();
        drawCrateAttributes();
}

void CrateAppImpl::OnImageMotion(wxMouseEvent& event){
        if(mousePressedInImageField){
                if(QRCodeCornerRDB->GetValue()){
                        QRCorner = event.GetPosition();
                }else if(CrateLineRDB->GetValue() || OppositeCornerRDB->GetValue()){
                        OppositeCorner = event.GetPosition();
                }else if(ZoomBox_radioBtn->GetValue()){

                        zoomWidth = event.GetX() - zoomX;
                        zoomHeight = event.GetY() - zoomY;
                }

                calculateFiducialPoints();
                drawCrateAttributes();
        }
}

void CrateAppImpl::OnColorSlider(wxScrollEvent& event){
        drawCrateAttributes();
}

void CrateAppImpl::OnZoomRadioButton(wxMouseEvent& event){
        if(!ZoomBox_radioBtn->GetValue()){
                ZoomCheckBox->SetValue(true);
                ZoomBox_radioBtn->SetValue(true);
                drawCrateAttributes();
        }
}

void CrateAppImpl::OnZoomChange(wxMouseEvent& event) {
        if (!ZoomCheckBox->GetValue()) {
                ZoomBox_radioBtn->SetValue(true);
                ZoomCheckBox->SetValue(true);
        } else {
                CrateLineRDB->SetValue(true);
                ZoomCheckBox->SetValue(false);
        }
        drawCrateAttributes();
}

void CrateAppImpl::OnZoom(wxCommandEvent& event) {
        if (zoomWidth != 0 && zoomHeight != 0) {
                zoomImage = image.Size(
                                wxSize(zoomX * Scale + zoomWidth * Scale,
                                                zoomY * Scale + zoomHeight * Scale),
                                wxPoint(zoomX * Scale, zoomY * Scale));

                zoomImage = (zoomImage.Rotate90(true)).Rotate90(true);
                zoomImage = zoomImage.Size(
                                wxSize(zoomWidth * Scale, zoomHeight * Scale),
                                wxPoint(0, 0));
                zoomImage = (zoomImage.Rotate90(true)).Rotate90(true);

                MessageLabel->SetLabel(
                                wxT("The values of the box and rotation line are reset"));
                wxSize imageMaxSize = this->GetSize();
                imageMaxSize.SetWidth(
                                imageMaxSize.GetWidth() - bSizer121->GetSize().GetWidth());
                imageMaxSize.SetHeight(
                                imageMaxSize.GetHeight() - MessageLabel->GetSize().GetHeight()
                                                - 20);

                double heightScale = (double) zoomImage.GetHeight()
                                / (double) imageMaxSize.GetHeight();
                double widthScale = (double) zoomImage.GetWidth()
                                / (double) imageMaxSize.GetWidth();

                if(widthScale < heightScale){
                        LargestScale = heightScale;
                }else{
                        LargestScale = widthScale;
                }

                QRCodeCornerLabel->SetLabel(wxString(("(0.00,0.00)"), wxConvLocal));
                OppositeCornerLabel->SetLabel(wxString(("(0.00,0.00)"), wxConvLocal));

                zoom = true;

                ZoomButton->Enable(false);
                OriginalImageButton->Enable(true);
                ZoomBox_radioBtn->Enable(false);
                CrateLineRDB->SetValue(true);

                UpdateImageField();

                this->Layout();
        } else {
                MessageLabel->SetLabel(wxT("Draw box to zoom into"));
        }
}

void CrateAppImpl::OnOriginal(wxCommandEvent& event) {
        MessageLabel->SetLabel(
                        wxT("The values of the box and rotation line are reset"));

        QRCorner = wxPoint(0, 0);
        OppositeCorner = wxPoint(0, 0);

        zoom = false;

        zoomX = 0;
        zoomY = 0;
        zoomWidth = 0;
        zoomHeight = 0;

        ZoomButton->Enable(true);
        ZoomBox_radioBtn->Enable(true);
        OriginalImageButton->Enable(false);

        drawCrateAttributes();
        this->Layout();
}

std::string CrateAppImpl::DoubleToStringFormatted(double d){
	std::stringstream ss;
	ss << std::fixed << std::setprecision(6) << d;
	return ss.str();
}

void CrateAppImpl::OnNextObjectButton(wxCommandEvent& event){
        SkipButton->Enable(false);

        if(QRCorner == wxPoint(0, 0) ||
                        OppositeCorner == wxPoint(0, 0)){
                MessageLabel->SetLabel(wxT("Corners are not defined"));
                CrateLineRDB->SetValue(true);
                CrateLineRDB->SetFocus();
                return;

        }

        if(QRCodeTextBox->GetValue() == wxT("") && ObjectTypeCombo->GetValue() != wxT("Marker")){
                QRCodeTextBox->SetFocus();
                MessageLabel->SetLabel(wxT("QR code value is not given"));
                return;
        }

        if (XMLOption == Edit && currentObjectNumber == 0) {

                SkipButton->Enable(false);
                //if images available within the xml need to overwritten
                foreach(boost::property_tree::ptree::value_type & imageValue,
                                pt.get_child("metadata"))
                {
                        //<xmlattr>.path is <image path="/home/...." />
                        boost::filesystem::path temp = imageValue.second.get("<xmlattr>.path", "");
                        if (temp.leaf() == (*imagePathsIt).leaf()) {
                                tempValue = &(imageValue.second);
                                //if the image name is found within the xml
                                AlreadyInXML = true;

                                imageValue.second.erase("property");
                                imageValue.second.erase("category");
                                imageValue.second.erase("object");
                                break;

                        }
                }
        }

        if (!AlreadyInXML && currentObjectNumber == 0) {
                tempValue = &(pt.add("metadata.image", ""));

                boost::filesystem::path p = *imagePathsIt;
                getPathFromChosenDir(p);
                tempValue->put("<xmlattr>.path", p);
        }

        if(currentObjectNumber == 0){
                boost::property_tree::ptree* property = &(tempValue->add("category", ""));
                property->put("<xmlattr>.name", "background");
                property->put("<xmlattr>.value", BackgroundComboBox->GetValue().ToAscii());
                property = &(tempValue->add("category", ""));
                property->put("<xmlattr>.name", "light");
                property->put("<xmlattr>.value", LightingComboBox->GetValue().ToAscii());
                property = &(tempValue->add("category", ""));
                property->put("<xmlattr>.name", "perspective");
                property->put("<xmlattr>.value", PerspectiveComboBox->GetValue().ToAscii());

        }

        QRCode = (std::string)QRCodeTextBox->GetValue().ToAscii();
        if(ObjectTypeCombo->GetValue() == wxT("Marker")){
                center.m_x = QRCorner.x;
                center.m_y = QRCorner.y;
        }

        if(zoom){
			center.m_x = center.m_x * LargestScale;
			fid1.m_x = fid1.m_x * LargestScale;
			fid2.m_x = fid2.m_x * LargestScale;
			fid3.m_x = fid3.m_x * LargestScale;

			center.m_y = center.m_y * LargestScale;
			fid1.m_y = fid1.m_y * LargestScale;
			fid2.m_y = fid2.m_y * LargestScale;
			fid3.m_y = fid3.m_y * LargestScale;

			center.m_x += zoomX* Scale;
			fid1.m_x += zoomX* Scale;
			fid2.m_x += zoomX* Scale;
			fid3.m_x += zoomX* Scale;

			center.m_y += zoomY* Scale;
			fid1.m_y += zoomY* Scale;
			fid2.m_y += zoomY* Scale;
			fid3.m_y += zoomY* Scale;
        }else{
			center.m_x = center.m_x * Scale;
			fid1.m_x = fid1.m_x * Scale;
			fid2.m_x = fid2.m_x * Scale;
			fid3.m_x = fid3.m_x * Scale;

			center.m_y = center.m_y * Scale;
			fid1.m_y = fid1.m_y * Scale;
			fid2.m_y = fid2.m_y * Scale;
			fid3.m_y = fid3.m_y * Scale;

        }

        boost::property_tree::ptree& object = tempValue->add("object", "");
        boost::property_tree::ptree* property = &(object.add("property", ""));
        property->put("<xmlattr>.name", "type");
        property->put("<xmlattr>.value", ObjectTypeCombo->GetValue().ToAscii());
        property = &(object.add("property", ""));
        property->put("<xmlattr>.name", "x");
        property->put("<xmlattr>.value", DoubleToStringFormatted(center.m_x));
        property = &(object.add("property", ""));
        property->put("<xmlattr>.name", "y");
        property->put("<xmlattr>.value", DoubleToStringFormatted(center.m_y));

        if(ObjectTypeCombo->GetValue() == wxT("Crate")){
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "fid1.x");
                property->put("<xmlattr>.value", DoubleToStringFormatted(fid1.m_x));
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "fid1.y");
                property->put("<xmlattr>.value", DoubleToStringFormatted(fid1.m_y));
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "fid2.x");
                property->put("<xmlattr>.value", DoubleToStringFormatted(fid2.m_x));
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "fid2.y");
                property->put("<xmlattr>.value", DoubleToStringFormatted(fid2.m_y));
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "fid3.x");
                property->put("<xmlattr>.value", DoubleToStringFormatted(fid3.m_x));
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "fid3.y");
                property->put("<xmlattr>.value", DoubleToStringFormatted(fid3.m_y));

        }else if(ObjectTypeCombo->GetValue() == wxT("QR code")){
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "marker1.x");
                property->put("<xmlattr>.value", DoubleToStringFormatted(fid1.m_x));
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "marker1.y");
                property->put("<xmlattr>.value", DoubleToStringFormatted(fid1.m_y));
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "marker2.x");
                property->put("<xmlattr>.value", DoubleToStringFormatted(fid2.m_x));
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "marker2.y");
                property->put("<xmlattr>.value", DoubleToStringFormatted(fid2.m_y));
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "marker3.x");
                property->put("<xmlattr>.value", DoubleToStringFormatted(fid3.m_x));
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "marker3.y");
                property->put("<xmlattr>.value", DoubleToStringFormatted(fid3.m_y));
        }

        if(ObjectTypeCombo->GetValue() != wxT("Marker")){
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "qrcode");
                property->put("<xmlattr>.value", QRCode);
        }else{
                property = &(object.add("property", ""));
                property->put("<xmlattr>.name", "radius");
                property->put("<xmlattr>.value", DoubleToStringFormatted(sqrt(pow(double(QRCorner.x - OppositeCorner.x), 2) + pow(double(QRCorner.y - OppositeCorner.y), 2)) * Scale));
        }

        boost::property_tree::xml_writer_settings<char> w('\t', 1);
        boost::property_tree::write_xml(xmlPath.string().c_str(), pt, std::locale(), w);
        MessageLabel->SetLabel(wxString(xmlPath.string().c_str(), wxConvLocal));

        currentObjectNumber++;

        QRCorner = wxPoint(0, 0);
        OppositeCorner = wxPoint(0, 0);

        QRCodeCornerLabel->SetLabel(wxT("(0.00,0.00)"));
        OppositeCornerLabel->SetLabel(wxT("(0.00,0.00)"));

        UpdateImageField();
        std::stringstream s;
        s << "Next image, current object number: " << currentObjectNumber + 1;
        MessageLabel->SetLabel(wxString(s.str().c_str(), wxConvLocal));
}

void CrateAppImpl::OnSkip(wxMouseEvent& event){

        QRCorner = wxPoint(0, 0);
        OppositeCorner = wxPoint(0, 0);

        zoomX = 0;
        zoomY = 0;
        zoomWidth = 0;
        zoomHeight = 0;

        zoom = false;

        QRCodeCornerLabel->SetLabel(wxT("(0.00,0.00)"));
        OppositeCornerLabel->SetLabel(wxT("(0.00,0.00)"));

        NextImage();
}

void CrateAppImpl::OnReset(wxCommandEvent& event){

        QRCorner = wxPoint(0, 0);
        OppositeCorner = wxPoint(0, 0);

        zoomX = 0;
        zoomY = 0;
        zoomWidth = 0;
        zoomHeight = 0;

        zoom = false;

        QRCodeCornerLabel->SetLabel(wxT("(0.00,0.00)"));
        OppositeCornerLabel->SetLabel(wxT("(0.00,0.00)"));

        if(currentObjectNumber > 0){
                foreach(boost::property_tree::ptree::value_type & imageValue, pt.get_child("metadata"))
                {
                        boost::filesystem::path temp = imageValue.second.get("<xmlattr>.path", "");
                        if (temp.leaf() == (*imagePathsIt).leaf()) {
                                tempValue = &(imageValue.second);
                                //if the image name is found within the xml
                                AlreadyInXML = true;

                                imageValue.second.erase("property");
                                imageValue.second.erase("category");
                                imageValue.second.erase("object");
                                break;

                        }
                }
                AlreadyInXML = true;
        }
        currentObjectNumber = 0;

        std::stringstream s;
        s << "Image reset, current object number: " << currentObjectNumber + 1;
        MessageLabel->SetLabel(wxString(s.str().c_str(), wxConvLocal));

        setCurrentImagePath(currentImagePath);
}

void CrateAppImpl::OnDoneButton(wxCommandEvent& event){
        this->Close(true);
}
