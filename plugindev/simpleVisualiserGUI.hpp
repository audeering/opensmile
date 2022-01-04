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


#ifndef __CSIMPLEVISUALISERGUI_HPP
#define __CSIMPLEVISUALISERGUI_HPP

#include <core/smileCommon.hpp>
#include <core/smileThread.hpp>
#include <core/dataSink.hpp>
#include <wx/wx.h>

#define COMPONENT_DESCRIPTION_CSIMPLEVISUALISERGUI "This is an example of a cDataSink descendant. It reads data from the data memory and prints it to the console. This component is intended as a template for developers."
#define COMPONENT_NAME_CSIMPLEVISUALISERGUI "cSimpleVisualiserGUI"


class paintDataEvent {
public:
  paintDataEvent(int _nvals=1) : val(NULL) {
    nVals = _nvals;
    if (_nvals > 0) {
      val = new int[_nvals];
    }
  }
  ~paintDataEvent() { if (val != NULL) delete[] val;  }

  int action; // type of plot
  int matMultiplier;
  int nVals;
  int *val;
  const char **colours;
  const char **names;
  FLOAT_DMEM scale0;
  FLOAT_DMEM offset0;
};



#include <wx/sizer.h>
 
class wxRtplotPanel : public wxPanel
    {
        //wxBitmap image;
        
        
    public:
        paintDataEvent *paintEventCur;
        paintDataEvent *paintEventLast;

        bool showImg;
        smileMutex wxMtx;
        //int curval, lastval;
        int curT, lastT;
        //const  char * featureName;
        //FLOAT_DMEM yscale100;

        wxRtplotPanel(wxFrame* parent);
        
        void paintEvent(wxPaintEvent & evt);
        void paintNow();
        
        void render(wxDC& dc);
        void renderMoving2dPlot(wxDC& dc);
        void renderMovingMatPlot(wxDC& dc);
        
        DECLARE_EVENT_TABLE()
    };
 
 
  


// the ID we'll use to identify our event
const int PLOT_UPDATE_ID = 100001;

class visFrame: public wxFrame
{
  //wxBitmap bmp;
  wxRtplotPanel * drawPane;   
  wxCheckBox *m_cb;

public:
    //FLOAT_DMEM yscale100;
    smileMutex wxMtx;

    visFrame(const wxString& title, const wxPoint& pos, const wxSize& size, smileMutex _wxMtx);

    void OnQuit(wxCommandEvent& event);
/*    void OnAbout(wxCommandEvent& event);*/

    // this is called when the event from the thread is received
    void onPlotUpdate(wxCommandEvent& evt)
    {
      //int val = evt.GetInt();

      // update the plot data (cyclic buffer)

      smileMutexLock(wxMtx);
      // replot
      drawPane->paintEventCur = (paintDataEvent *)evt.GetClientData();
      smileMutexUnlock(wxMtx);

/*
      drawPane->yscale100 = yscale100;
      drawPane->featureName = evt.GetString();
      drawPane->curval = val;
      */
      drawPane->showImg = true;
      drawPane->paintNow();

      
    }


    

    DECLARE_EVENT_TABLE()
};


class visApp: public wxApp
{
  virtual bool OnInit() override;
public:
  //FLOAT_DMEM yscale100;
  smileMutex wxMtx;

  visFrame * vframe;
  
};

#define VIS_ACT_MOVING2DPLOT 1
#define VIS_ACT_MOVINGMATPLOT 2

class cSimpleVisualiserGUI : public cDataSink {
  private:
    int action; int fullinput;
    int nInputs;
    int *inputsIdx;
    FLOAT_DMEM *inputScale;
    FLOAT_DMEM *inputOffset;

    const char **inputNames;
    const char **inputColours;
    
    //wxCommandEvent &myevent; //( wxEVT_COMMAND_TEXT_UPDATED, VAD_UPDATE_ID );
    //int old;

    smileThread guiThr;
    //gcroot<openSmilePluginGUI::TestForm^> fo;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    int getElidxFromName(const char *_name); 

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    //FLOAT_DMEM yscale100;

    int matMultiplier;
    int terminate;
    visApp* pApp;
    smileMutex wxMtx;

    cSimpleVisualiserGUI(const char *_name);

    virtual ~cSimpleVisualiserGUI();
};




#endif // __CSIMPLEVISUALISERGUI
