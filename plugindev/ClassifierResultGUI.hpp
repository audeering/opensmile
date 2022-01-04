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


#ifndef __CCLASSIFIERRESULTGUI_HPP
#define __CCLASSIFIERRESULTGUI_HPP

#include <core/smileCommon.hpp>
#include <core/smileComponent.hpp>
#include <core/smileThread.hpp>
#include <wx/wx.h>

#define COMPONENT_DESCRIPTION_CCLASSIFIERRESULTGUI "This is an example of a cDataSink descendant. It reads data from the data memory and prints it to the console. This component is intended as a template for developers."
#define COMPONENT_NAME_CCLASSIFIERRESULTGUI "cClassifierResultGUI"



#include <wx/sizer.h>
 
class wxMultiImagePanel : public wxPanel
    {
      const char **clsName;
      int classActive;
      int nClasses;
      wxBitmap *image;
      float *confidence;

      int nRegression;
      const char ** regressionText;
      float * regressionValue;

    public:
        wxMultiImagePanel(wxFrame* parent,  int _nClasses=0, int nRegr=0);
        
        void loadClassImage(int clsIdx, const char *clsname, wxString file, wxBitmapType format);
        void setRegressionText(int nr, const char *text) {
          if (regressionText != NULL && nr>=0 && nr < nRegression) {
            regressionText[nr] = text;
          }
        }

        void paintEvent(wxPaintEvent & evt);
        void paintNow();
        
        void renderGauge(wxDC&  dc, int x, int y, int w, int h, int val, const char * label, const char *col="blue");
        void render(wxDC& dc);
        
        void setClassResult(int nr) {
          if (nr >= 0 && nr < nClasses) {
            classActive = nr;
          }
        }
        void setClassConfidence(int idx, float conf) {
          if (confidence != NULL && idx >= 0 && idx < nClasses) {
            confidence[idx] = conf;
          }
        }
        void setRegressionVal(int nr, float val) {
          if (regressionValue != NULL && nr >= 0 && nr < nRegression) {
            regressionValue[nr] = val; 
          }
        }

          ~wxMultiImagePanel() {
            if (regressionText != NULL) free(regressionText);
            if (clsName != NULL) free(clsName);
            if (regressionValue != NULL) delete[] regressionValue;
            if (confidence != NULL) delete[] confidence;
            if (image != NULL) delete[] image;
          }

        DECLARE_EVENT_TABLE()
    };
 
 
class classificationEvent {
public:
  classificationEvent(int _nClasses=0, int _maxClsIdx=0, const char *_classname=NULL, int nRegr=0) : 
      confidences(NULL), nClasses(_nClasses), maxClsIdx(_maxClsIdx), classname(_classname), nRegression(nRegr), regUpdate(-1)
      {
        if (nClasses > 0) { confidences = new float[nClasses]; }
        if (nRegression > 0) { regressionValue = new float[nRegression]; }
      }
      ~classificationEvent() { 
        if (confidences != NULL) delete[] confidences;
        if (regressionValue != NULL) delete[] regressionValue;
      }

  const char *classname;
  int maxClsIdx;
  int nClasses;
  int nRegression;
  int regUpdate; // the index to update, set to -1 to update all
  float *confidences;
  float *regressionValue;
  
};
 

// the ID we'll use to identify our event
const int RESULT_UPDATE_ID = 100002;

class ClsResultFrame: public wxFrame
{
  //wxBitmap bmp;
    
  wxCheckBox *m_cb;

public:
    wxMultiImagePanel * drawPane; 

    ClsResultFrame(const wxString& title, const wxPoint& pos, const wxSize& size, int nCls, int nReg);

/*    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);*/

    // this is called when the event from the thread is received
    void onResultUpdate(wxCommandEvent& evt)
    {
      //smileMutexLock(wxMtx);
      // replot
      classificationEvent *ce = (classificationEvent *)evt.GetClientData();
      if (ce->nClasses > 0) {
        drawPane->setClassResult(ce->maxClsIdx);
      } else {
      if (ce->regUpdate >= 0) {
            printf("event: clsidx = %i  val=%f\n",ce->regUpdate,ce->regressionValue[ce->regUpdate]);

        drawPane->setRegressionVal(ce->regUpdate, ce->regressionValue[ce->regUpdate]);
      } else {
        int i;
        for (i=0; i<ce->nRegression; i++) {
          drawPane->setRegressionVal(i, ce->regressionValue[i]);
        }
      }
      }

      int i;
      for (i=0; i<ce->nClasses; i++) {
        drawPane->setClassConfidence(i,ce->confidences[i]);
      }

      drawPane->paintNow();
      //smileMutexUnlock(wxMtx);
    }

    DECLARE_EVENT_TABLE()
};

class cClassifierResultGUI;

class ClsResultApp: public wxApp
{
  virtual bool OnInit() override;
public:
  cClassifierResultGUI * smileObj;
  ClsResultFrame * mframe;
  
};


class cClassifierResultGUI : public cSmileComponent {
  private:
    smileThread guiThr;
    
    //gcroot<openSmilePluginGUI::TestForm^> fo;

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    int getClassIndex(const char *name);
    int getRegressionIndex(const char *name);

    virtual void myFetchConfig() override;
    //virtual int myConfigureInstance() override;
    virtual int myFinaliseInstance() override;
    virtual eTickResult myTick(long long t) override;
    virtual int processComponentMessage(cComponentMessage *_msg) override;

  public:
    SMILECOMPONENT_STATIC_DECL
    
    int nClasses;
    int nRegression;
    const char **className;
    const char **imageFile;
    const char **regressionName;

    smileMutex wxMtx;
    ClsResultApp* pApp;
    cClassifierResultGUI(const char *_name);

    virtual ~cClassifierResultGUI();
};




#endif // __CCLASSIFIERRESULTGUI_HPP
