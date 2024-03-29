#include <wx/wx.h>
#include <huniplacer/huniplacer.h>
#include <huniplacer_gui/utils.h>
#include <huniplacer_gui/huniplacer_frame_impl.h>
#include <cstdio>
#include <typeinfo>
#include <iostream>
#include <sstream>
#include <vector>
#include <string.h>

extern huniplacer_gui::huniplacer_frame_impl* frame;

namespace huniplacer_gui
{
    huniplacer_frame_impl::huniplacer_frame_impl(void) :
        huniplacer_frame(NULL),
        motors(NULL),
        robot(NULL),
        cur_x(0), cur_y(0), cur_z(-150)
    {
        ik_model = new huniplacer::inverse_kinematics_impl(
            huniplacer::measures::BASE,
            huniplacer::measures::HIP,
            huniplacer::measures::EFFECTOR,
            huniplacer::measures::ANKLE,
            huniplacer::measures::HIP_ANKLE_ANGLE_MAX);

        button_off->Enable(false);
        button_on->Enable(false);
        button_disconnect->Enable(false);
        update_pos_txtfields();
        //update_z_slider();
        pos_panel->Refresh();
        pos_panel_side->Refresh();
        txtbox_speed->SetValue(wxT("360"));

    }

    huniplacer_frame_impl::~huniplacer_frame_impl(void)
    {
        destroy_deltarobot();
        delete ik_model;
    }

    void huniplacer_frame_impl::init_deltarobot(const char* device)
    {
        if(motors == NULL && robot == NULL)
        {
            modbus_t* rtu = modbus_new_rtu(
                device,
                crd514_kd::rtu_config::BAUDRATE,
                crd514_kd::rtu_config::PARITY,
                crd514_kd::rtu_config::DATA_BITS,
                crd514_kd::rtu_config::STOP_BITS);
            double deviation[] = {huniplacer::measures::MOTOR1_DEVIATION,
            		huniplacer::measures::MOTOR2_DEVIATION,
            		huniplacer::measures::MOTOR3_DEVIATION
            };
            motors = new huniplacer::steppermotor3(
                rtu,
                huniplacer::measures::MOTOR_ROT_MIN,
                huniplacer::measures::MOTOR_ROT_MAX,
                huniplacer_frame_impl::motionthread_exhandler,
                deviation
                );
            robot = new huniplacer::deltarobot(*ik_model, *motors);
            lab_status->SetLabel(wxT("Status: creating boundaries"));
            lab_status->Update();
            robot->generate_boundaries(0.5);
            lab_status->SetLabel(wxT("Status: connected, off"));
        }
        else
        {
            popup_warn(wxT("deltarobot already initialized"));
        }
    }

    void huniplacer_frame_impl::destroy_deltarobot(void)
    {
        if(motors != NULL && robot != NULL)
        {
            delete robot;
            delete motors;
            robot = NULL;
            motors = NULL;
            lab_status->SetLabel(wxT("Status: disconnected"));
        }
    }

    void huniplacer_frame_impl::popup_warn(const wxString& msg)
    {
        wxMessageBox(msg, wxT("warning"));
    }

    void huniplacer_frame_impl::popup_err(const wxString& msg)
    {
        wxMessageBox(msg, wxT("error"));
    }

    void huniplacer_frame_impl::motionthread_exhandler(std::runtime_error& err)
    {
        //popup_err(wxT("an error occured in the steppermotor motion thread"));
        fprintf(stderr,
            "an error of type %s occured in the motion thread\n"
            "what(): %s\n",
            typeid(err).name(), err.what());
    }

    void huniplacer_frame_impl::update_pos_txtfields(void)
    {
        wxString sx, sy, sz;
        sx << cur_x;
        sy << cur_y;
        sz << cur_z;

        txtbox_x->SetValue(sx);
        txtbox_y->SetValue(sy);
        txtbox_z->SetValue(sz);
    }

	/*
    void huniplacer_frame_impl::update_z_slider(void)
    {
        double z = utils::convert_scale(
            0,
            slider_z_pos->GetMax(),
            huniplacer::measures::MIN_Z,
            huniplacer::measures::MAX_Z,
            cur_z);
        slider_z_pos->SetValue(z);
    }
*/
    bool huniplacer_frame_impl::try_move(double x, double y, double z)
    {
    	if(robot == NULL || motors == NULL )
    	{
    		return false;
    	}

        printf(
            "try_move called with:\n"
            "  x: %lf\n"
            "  y: %lf\n"
            "  z: %lf\n",
            x, y, z);

        if(x == cur_x && y == cur_y && z == cur_z)
        {
        	return true;
        }

        try
        {
            double speed;
            if(!txtbox_speed->GetValue().ToDouble(&speed) || speed < 20)
            {
                speed = 20;
                wxString s;
                s << speed;
                txtbox_speed->SetValue(s);
            }

            printf("speed = %lf\n", speed);
            robot->moveto(huniplacer::point3(x, y, z), speed);
            cur_x = x;
            cur_y = y;
            cur_z = z;

            return true;
        }
        catch(std::runtime_error& err)
        {
            wxString s;
            s << wxString("runtime error of type ", wxConvLocal)
              << wxString(typeid(err).name(), wxConvLocal)
              << wxString("\n", wxConvLocal)
              << wxString("what(): ", wxConvLocal) << wxString(err.what(), wxConvLocal);
            popup_err(s);
        }

        return false;
    }

    //events
    void huniplacer_frame_impl::pos_panelOnLeftDown(wxMouseEvent& event)
    {
		int w, h;
		int px, py;
		pos_panel->GetSize(&w, &h);
		event.GetPosition(&px, &py);

		double x = utils::convert_scale(
			huniplacer::measures::MIN_X,
			huniplacer::measures::MAX_X,
			0,
			w,
			px);
		double y = utils::convert_scale(
				huniplacer::measures::MIN_Y,
				huniplacer::measures::MAX_Y,
				0,
				h,
				py);

		if(try_move(x, y, cur_z))
		{
			update_pos_txtfields();
			pos_panel->Refresh();
			pos_panel_side->Refresh();
		}
    }

    void huniplacer_frame_impl::pos_panel_sideOnLeftDown(wxMouseEvent& event)
     {
 		int w, h;
 		int px, py;
 		pos_panel_side->GetSize(&w, &h);
 		event.GetPosition(&px, &py);

 		double x = utils::convert_scale(
 			huniplacer::measures::MIN_X,
 			huniplacer::measures::MAX_X,
 			0,
 			w,
 			px);
 		double z = utils::convert_scale(
 				huniplacer::measures::MIN_Z,
 				huniplacer::measures::MAX_Z,
 				h,
 				0,
 				py);

 		if(try_move(x, cur_y, z))
 		{
 			update_pos_txtfields();
 			pos_panel->Refresh();
 			pos_panel_side->Refresh();
 		}
     }


    void huniplacer_frame_impl::pos_panelOnPaint(wxPaintEvent& event)
    {
        //clear
        wxPaintDC dc(pos_panel);
        dc.Clear();
        dc.SetPen(wxPen(wxColour(0, 10, 100)));
        dc.SetBrush(wxBrush(pos_panel->GetBackgroundColour()));

        int w, h;
        pos_panel->GetSize(&w, &h);

        if(robot != NULL && robot->has_boundaries())
        {
            huniplacer::effector_boundaries* boundaries = robot->get_boundaries();
			const bool* boundaries_plane = boundaries->get_bitmap();
			int width = boundaries->get_width();
			int depth = boundaries->get_depth();
			//int height = boundaries->get_height();
			double voxel_size = boundaries->get_voxel_size();

			for(int x = 0; x < w; x++)
			{
				for(int y = 0; y < h; y++)
				{
					int boundary_x = (double)x * ((double)width / (double)w);
					int boundary_y = (double)y * ((double)depth / (double)h);
					int boundary_z = (cur_z - huniplacer::measures::MIN_Z) / voxel_size;
					int index = boundary_x + boundary_y * width + boundary_z * depth * width;

					if(boundaries_plane[index])
					{
						dc.DrawPoint(x, y);
					}
				}
			}
        }

        //draw lines
        dc.SetPen(wxPen(pos_panel->GetForegroundColour()));
        dc.SetBrush(wxBrush(pos_panel->GetBackgroundColour()));

        double x = utils::convert_scale(
            0, w,
            huniplacer::measures::MIN_X, huniplacer::measures::MAX_X,
            cur_x);
        double y = utils::convert_scale(
            0, h,
            huniplacer::measures::MIN_Y, huniplacer::measures::MAX_Y,
            cur_y);
        dc.DrawLine(wxPoint(0, y), wxPoint(w-1, y));
        dc.DrawLine(wxPoint(x, 0), wxPoint(x, h-1));
    }

    void huniplacer_frame_impl::pos_panel_sideOnPaint(wxPaintEvent& event)
	{
        //clear
         wxPaintDC dc(pos_panel_side);
         dc.Clear();
         dc.SetPen(wxPen(wxColour(0, 10, 100)));
         dc.SetBrush(wxBrush(pos_panel_side->GetBackgroundColour()));

         int w, h;
         pos_panel_side->GetSize(&w, &h);

         if(robot != NULL && robot->has_boundaries())
         {
             huniplacer::effector_boundaries* boundaries = robot->get_boundaries();
 			const bool* boundaries_plane = boundaries->get_bitmap();
 			int width = boundaries->get_width();
 			int depth = boundaries->get_depth();
 			int height = boundaries->get_height();
 			double voxel_size = boundaries->get_voxel_size();

 			for(int x = 0; x < w; x++)
 			{
 				for(int y = 0; y < h; y++)
 				{
 					int boundary_x = (double)x * ((double)width / (double)w);
 					int boundary_y = (cur_y - huniplacer::measures::MIN_Y) / voxel_size;
 					int boundary_z = (double)y * ((double)height / (double)h);
					int index = boundary_x + boundary_y * width + boundary_z * depth * width;

					if(boundaries_plane[index])
					{
						dc.DrawPoint(x, h - y);
					}
 				}
 			}
         }

         //draw lines
         dc.SetPen(wxPen(pos_panel->GetForegroundColour()));
         dc.SetBrush(wxBrush(pos_panel->GetBackgroundColour()));

         double x = utils::convert_scale(
             0, w,
             huniplacer::measures::MIN_X, huniplacer::measures::MAX_X,
             cur_x);
         double y = utils::convert_scale(
             h, 0,
             huniplacer::measures::MIN_Z, huniplacer::measures::MAX_Z,
             cur_z);
         dc.DrawLine(wxPoint(0, y), wxPoint(w-1, y));
         dc.DrawLine(wxPoint(x, 0), wxPoint(x, h-1));
	}

    /*
    void huniplacer_frame_impl::slider_z_posOnLeftUp(wxMouseEvent& event)
    {

        double z = utils::convert_scale(
            huniplacer::measures::MIN_Z,
            huniplacer::measures::MAX_Z,
            0,
            slider_z_pos->GetMax(),
            slider_z_pos->GetValue());

        if(try_move(cur_x, cur_y, z))
        {
            update_pos_txtfields();
        }

        pos_panel->Refresh();

        event.Skip();
    }
    */

    void huniplacer_frame_impl::button_moveOnButtonClick(wxCommandEvent& event)
    {
        double x, y, z;
        txtbox_x->GetValue().ToDouble(&x);
        txtbox_y->GetValue().ToDouble(&y);
        txtbox_z->GetValue().ToDouble(&z);
        if(try_move(x, y, z))
        {
            //update_z_slider();
            pos_panel->Refresh();
            pos_panel_side->Refresh();
        }
    }

    void huniplacer_frame_impl::button_connectOnButtonClick(wxCommandEvent& event)
    {
        wxFileDialog browser(this);

        if(browser.ShowModal() == wxID_OK)
        {
            std::stringstream ss;
            ss << browser.GetPath().ToAscii();

            init_deltarobot(ss.str().c_str());

            button_disconnect->Enable(true);
            button_connect->Enable(false);
            button_on->Enable(true);

            pos_panel->Refresh();
            pos_panel_side->Refresh();
        }
    }

    void huniplacer_frame_impl::button_disconnectOnButtonClick(wxCommandEvent& event)
    {
    		destroy_deltarobot();
    		button_disconnect->Enable(false);
    		button_connect->Enable(true);
    		button_off->Enable(false);
    		button_on->Enable(false);
    }

    void huniplacer_frame_impl::button_onOnButtonClick(wxCommandEvent& event)
    {
        if(robot == NULL)
        {
            popup_err(wxT("robot not yet initialized"));
            button_off->Enable(false);
            button_on->Enable(true);
        }
        else
        {
            lab_status->SetLabel(wxT("Status: on"));
            robot->power_on();
            button_on->Enable(false);
            button_off->Enable(true);
        }
    }

    void huniplacer_frame_impl::button_offOnButtonClick(wxCommandEvent& event)
    {
        if(robot == NULL)
        {
            popup_err(wxT("robot not yet initialized"));
            button_on->Enable(false);
            button_off->Enable(true);
        }
        else
        {
            lab_status->SetLabel(wxT("Status: off"));
            robot->stop();
            robot->power_off();
            button_off->Enable(false);
            button_on->Enable(true);
        }
    }
}
