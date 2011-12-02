#include <huniplacer_gui/huniplacer_frame_impl.h>
#include <wx/wx.h>
#include <huniplacer_gui/utils.h>
#include <huniplacer/huniplacer.h>
#include <cstdio>
#include <typeinfo>
#include <iostream>
#include <sstream>
#include <bitset>

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
        boundaries = effector_boundaries::generate_effector_boundaries(ik_model, 1);
        init_deltarobot("/dev/ttyS0");
        button_off->Enable(false);
        button_on->Enable(false);
        update_pos_txtfields();
        update_z_slider();
        pos_panel->Refresh();
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
            motors = new huniplacer::steppermotor3(
                rtu,
                huniplacer::measures::MOTOR_ROT_MIN,
                huniplacer::measures::MOTOR_ROT_MAX,
                huniplacer_frame_impl::motionthread_exhandler);
            robot = new huniplacer::deltarobot(*ik_model, *motors);
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
        }
        else
        {
            popup_warn(wxT("deltarobot not yet initialized"));
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

    bool huniplacer_frame_impl::try_move(double x, double y, double z)
    {
        printf(
            "try_move called with:\n"
            "  x: %lf\n"
            "  y: %lf\n"
            "  z: %lf\n",
            x, y, z);

        try
        {
            double speed;
            if(txtbox_speed->GetValue().ToDouble(&speed))
            {
                speed = 360;
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
        }
    }

    void huniplacer_frame_impl::pos_panelOnPaint(wxPaintEvent& event)
    {
        //clear
        wxPaintDC dc(pos_panel);
        dc.SetPen(wxPen(pos_panel->GetForegroundColour()));
        dc.SetBrush(wxBrush(pos_panel->GetBackgroundColour()));
        dc.Clear();

        int w, h;
        pos_panel->GetSize(&w, &h);

        //draw range
        //TODO draw range ;)
        std::bitset &boundaries_plane = boundaries.get_bitmap();
        int width = boundaries.get_width();
        int depth = boundaries.get_depth();
        int height = boundaries.get_height();
        double voxel_size = boundaries.get_voxel_size();
        for(int x = 0; x < w; x++)
        {
        	for(int y = 0; y < h; y++)
			{
        		double boundary_x = (double)x * (double)(width / w);
        		double boundary_y = (double)y * (double)(depth / h);
        		double boundary_z = (cur_z - measures::MIN_Z) / voxel_size;
        		if(boundaries_plane[boundary_x + boundary_y * width + boundary_z * depth * width])
        		{
        			dc.DrawPoint(x, y);
        		}
			}
        }

        //draw lines
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

        event.Skip();
    }

    void huniplacer_frame_impl::button_moveOnButtonClick(wxCommandEvent& event)
    {
        double x, y, z;
        txtbox_x->GetValue().ToDouble(&x);
        txtbox_y->GetValue().ToDouble(&y);
        txtbox_z->GetValue().ToDouble(&z);
        if(try_move(x, y, z))
        {
            update_z_slider();
            pos_panel->Refresh();
        }
    }

    void huniplacer_frame_impl::button_circleOnButtonClick(wxCommandEvent& event)
    {
        //TODO implement
        puts("circle button click");
    }

    void huniplacer_frame_impl::button_connectOnButtonClick(wxCommandEvent& event)
    {
        wxFileDialog browser(this);
        if(browser.ShowModal() == wxID_OK)
        {
            //TODO construct deltarobot
            printf("[unicode] browser.GetPath = %ls\n", browser.GetPath().c_str());
            printf("[ASCII]   browser.GetPath = %s\n", browser.GetPath().c_str());
            std::stringstream ss;
            ss << browser.GetPath().c_str();
            printf("[ASCII] ss.str = %s\n", ss.str().c_str());
        }
    }

    void huniplacer_frame_impl::button_disconnectOnButtonClick(wxCommandEvent& event)
    {
        destroy_deltarobot();
    }

    void huniplacer_frame_impl::button_onOnButtonClick(wxCommandEvent& event)
    {
        if(robot == NULL)
        {
            popup_err(wxT("robot not yet initialized"));
            button_on->Enable(true);
            button_off->Enable(true);
        }
        else
        {
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
            button_on->Enable(true);
            button_off->Enable(true);
        }
        else
        {
            robot->power_off();
            button_on->Enable(true);
            button_off->Enable(false);
        }
    }
}
