/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


/*  openSMILE component:

example dataSink:
reads data from data memory and outputs it to console/logfile (via smileLogger)
this component is also useful for debugging

*/


#ifndef __CEXAMPLEGUIPLUGINSINK_HPP
#define __CEXAMPLEGUIPLUGINSINK_HPP

#include <core/smileCommon.hpp>
#include <core/smileThread.hpp>
#include <core/dataSink.hpp>
#include <wx/wx.h>

#define COMPONENT_DESCRIPTION_CEXAMPLEGUIPLUGINSINK "This is an example of a cDataSink descendant. It reads data from the data memory and prints it to the console. This component is intended as a template for developers."
#define COMPONENT_NAME_CEXAMPLEGUIPLUGINSINK "cExampleGuipluginSink"



#include <wx/sizer.h>
 
class wxImagePanel : public wxPanel
    {
        wxBitmap image;
        
        
    public:
        bool showImg;

        wxImagePanel(wxFrame* parent, wxString file, wxBitmapType format);
        
        void paintEvent(wxPaintEvent & evt);
        void paintNow();
        
        void render(wxDC& dc);
        
        DECLARE_EVENT_TABLE()
    };
 
 
 
 

// the ID we'll use to identify our event
const int VAD_UPDATE_ID = 100000;

class MyFrame: public wxFrame
{
  //wxBitmap bmp;
  wxImagePanel * drawPane;   
  wxCheckBox *m_cb;

public:
    
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

/*    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);*/

    // this is called when the event from the thread is received
    void onVadUpdate(wxCommandEvent& evt)
    {
      // get the number sent along the event and use it to update the GUI
      if (evt.GetInt()==1) {
        //m_cb->SetValue( true );
        drawPane->showImg = true;
        drawPane->paintNow();
      } else {
        //m_cb->SetValue( false );
        drawPane->showImg = false;
        drawPane->paintNow();
      }
    }


    

    DECLARE_EVENT_TABLE()
};


class MyApp: public wxApp
{
  virtual bool OnInit() override;
public:
  MyFrame * mframe;
  
};


class cExampleGuipluginSink : public cDataSink {
  private:
    const char *filename;
    FILE * fHandle;
    int lag;
    //wxCommandEvent &myevent; //( wxEVT_COMMAND_TEXT_UPDATED, VAD_UPDATE_ID );
    int old;

    smileThread guiThr;
    //gcroot<openSmilePluginGUI::TestForm^> fo;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    MyApp* pApp;
    cExampleGuipluginSink(const char *_name);

    virtual ~cExampleGuipluginSink();
};




#endif // __CEXAMPLEGUIPLUGINSINK_HPP
