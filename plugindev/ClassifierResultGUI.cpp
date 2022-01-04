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


#include <classifierResultGUI.hpp>

#define MODULE "cClassifierResultGUI"


SMILECOMPONENT_STATICS(cClassifierResultGUI)

SMILECOMPONENT_REGCOMP(cClassifierResultGUI)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CCLASSIFIERRESULTGUI;
  sdescription = COMPONENT_DESCRIPTION_CCLASSIFIERRESULTGUI;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("classes","names of classification classes",(const char *)NULL,ARRAY_TYPE);
    ct->setField("regression","names of regression targets",(const char *)NULL,ARRAY_TYPE);

    ct->setField("images","The names of the image files to load for each class (array).",(const char *)NULL,ARRAY_TYPE);
    //ct->setField("showRegval","Show a regression value computed from the confidences (if showRegval=2), or which was received via the smileMessage (showRegval=1), or not at all (showRegval=0).",0);
  )

  SMILECOMPONENT_MAKEINFO(cClassifierResultGUI);
}


//SMILECOMPONENT_CREATE(cClassifierResultGUI)
cSmileComponent * cClassifierResultGUI::create(const char*_instname) { 
  cSmileComponent *c = new cClassifierResultGUI(_instname); 
  if (c!=NULL) c->setComponentInfo(COMPONENT_NAME_CCLASSIFIERRESULTGUI,COMPONENT_DESCRIPTION_CCLASSIFIERRESULTGUI); 
  return c; 
}

//--- wx ---



/*
enum
{
    ID_Quit = 1,
    ID_About,
};

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(ID_Quit,  MyFrame::OnQuit)
    EVT_MENU(ID_About, MyFrame::OnAbout)
END_EVENT_TABLE()
*/



wxMultiImagePanel::wxMultiImagePanel(wxFrame* parent,  int _nClasses, int nRegr) :
wxPanel(parent), nClasses(_nClasses), image(NULL), confidence(NULL), clsName(NULL), regressionValue(NULL), regressionText(NULL), nRegression(nRegr)
{
    // load the file... ideally add a check to see if loading was successful
    //image.LoadFile(file, format);
  if (nClasses > 0) {
    image = new wxBitmap[nClasses];
    confidence = new float[nClasses];
    clsName = (const char **)calloc(1,sizeof(const char*)*nClasses);
  }
  if (nRegr > 0) {
    regressionValue = new float[nRegression];
    regressionText =  (const char **)calloc(1,sizeof(const char*)*nRegression);
  }
}
 
void wxMultiImagePanel::loadClassImage(int clsIdx, const char *clsname, wxString file, wxBitmapType format)
{
  if (image != NULL && clsIdx >= 0 && clsIdx < nClasses) {
    image[clsIdx].LoadFile(file, format);
  }
  if (clsName != NULL && clsIdx >= 0 && clsIdx < nClasses) {
    clsName[clsIdx] = clsname;
  }
}

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */
 
void wxMultiImagePanel::paintEvent(wxPaintEvent & evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxPaintDC dc(this);
    render(dc);
}
 
/*
 * Alternatively, you can use a clientDC to paint on the panel
 * at any time. Using this generally does not free you from
 * catching paint events, since it is possible that e.g. the window
 * manager throws away your drawing when the window comes to the
 * background, and expects you will redraw it when the window comes
 * back (by sending a paint event).
 */
void wxMultiImagePanel::paintNow()
{
    // depending on your system you may need to look at double-buffered dcs
    wxClientDC dc(this);
    render(dc);
}
 
// render a simple meter gauge (vertical)
// val = 0..100 (percentage of filling)
void wxMultiImagePanel::renderGauge(wxDC&  dc, int x, int y, int w, int h, int val, const char * label, const char *col)
{
  dc.SetPen(wxPen(wxColour("black")));
  dc.SetBrush(wxBrush(wxColour("white")));
  dc.DrawRectangle(x,y,w,h);
  int fill=0;
  fill = (int)(((float)val / 100.0) * (float)w);
  if (fill >= w) fill = w-1;
  if (fill > 1) {
    dc.DrawLine(x+fill,y+1,x+fill,y+h-1);
    dc.SetBrush(wxBrush(wxColour(col)));
    dc.FloodFill(x+2,y+2,wxColour("black"),wxFLOOD_BORDER);
    dc.SetPen(wxPen(wxColour(col)));
    dc.DrawLine(x+fill,y+1,x+fill,y+h-1);
  } else if  (fill == 1) {
    dc.SetPen(wxPen(wxColour(col)));
    dc.DrawLine(x+fill,y+1,x+fill,y+h-1);
  }

  if (label != NULL) {
    dc.DrawText(label,x+w+10,y);
  }
}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void wxMultiImagePanel::render(wxDC&  dc)
{
  int imgX = 50;
  int imgY = 70;
  int imgW = 200;
  int imgH = 200;

  // position (upper left) of first gauge.
  int regGaugeX = 50;
  int regGaugeY = imgY+imgH+50;
  int gaugeW = 100;
  int gaugeH = 20;
  int gaugeSpace = 5;

  // position (upper left) of first gauge.
  int confGaugeX = 50+imgW+25;
  int confGaugeY = imgY+50;
 

  dc.Clear();

  //regressison value
  if (nRegression > 0) {
    int i;
    dc.DrawText("Regression result(s):",regGaugeX,regGaugeY-25);
    for (i=0; i<nRegression; i++) {
      renderGauge(dc, regGaugeX, regGaugeY+(gaugeH+gaugeSpace)*i, gaugeW, gaugeH, regressionValue[i], regressionText[i]);
    }
  }

  // class confidence gauges
  float ctr=0.0;
  if (nClasses > 0) {
    int i;
    dc.DrawText("Class confidences:",confGaugeX,confGaugeY-25);
    for (i=0; i<nClasses; i++) {
      renderGauge(dc, confGaugeX, confGaugeY+(gaugeH+gaugeSpace)*i, gaugeW, gaugeH, (int)(confidence[i]*100.0), clsName[i]);
      ctr += confidence[i]*(float)(i+1);
    }
    ctr -= 1.0;
    ctr /= 2.0;
  }

  int _classActive = classActive;
  /// make this configurable!
  _classActive = (int)floor(ctr*(float)nClasses);
  //// 
  if (nClasses > 0 && _classActive >= 0 && _classActive < nClasses && image != NULL) {
    dc.DrawBitmap( image[_classActive], imgX, imgY, false );
    if (clsName != NULL) {
      dc.DrawText("Classification result:", imgX,imgY-40);
      dc.DrawText(clsName[_classActive], imgX+10, imgY-20);
    }
  }

  // confidence centroid...
  dc.DrawText("Centroid of class confidences (pseudo regression:)",confGaugeX, confGaugeY+(gaugeH+gaugeSpace)*nClasses+10);
  renderGauge(dc, confGaugeX, confGaugeY+(gaugeH+gaugeSpace)*nClasses+30, gaugeW, gaugeH, (int)(ctr*100.0), "<- level of interest", "green");
  
}


// catch the event from the thread
BEGIN_EVENT_TABLE(ClsResultFrame, wxFrame)
EVT_COMMAND  (RESULT_UPDATE_ID, wxEVT_COMMAND_TEXT_UPDATED, ClsResultFrame::onResultUpdate)
END_EVENT_TABLE()


BEGIN_EVENT_TABLE(wxMultiImagePanel, wxPanel)
// catch paint events
EVT_PAINT(wxMultiImagePanel::paintEvent)
END_EVENT_TABLE()

const int ID_CHECKBOX = 100;


bool ClsResultApp::OnInit()
{
    // make sure to call this first
    wxInitAllImageHandlers();


    mframe = new ClsResultFrame( _("TUM openSMILE classifier demo output"), wxPoint(50, 50), wxSize(650, 350), smileObj->nClasses, smileObj->nRegression );
    mframe->Show(true);
    int i;
    for (i=0; i<smileObj->nClasses ; i++) {
      mframe->drawPane->loadClassImage(i,smileObj->className[i],smileObj->imageFile[i],wxBITMAP_TYPE_JPEG);
      
    }
    for (i=0; i<smileObj->nRegression ; i++) {
      mframe->drawPane->setRegressionText(i,smileObj->regressionName[i]);
    }
    SetTopWindow(mframe);
    return true;
}

ClsResultFrame::ClsResultFrame(const wxString& title, const wxPoint& pos, const wxSize& size, int nCls, int nReg)
       : wxFrame(NULL, wxID_ANY, title, pos, size)
{
    //wxPanel *panel = new wxPanel(this, wxID_ANY);
    
    if (nCls < 0) nCls = 0;
    if (nReg < 0) nReg = 0;

    //wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    drawPane = new wxMultiImagePanel( this, nCls, nReg );

    // wxT("vo.jpg"), wxBITMAP_TYPE_JPEG

    //sizer->Add(drawPane, 10, wxEXPAND);
    //this->SetSizer(sizer);


   // m_cb = new wxCheckBox(panel, ID_CHECKBOX, wxT("Voice activity"), 
   //                     wxPoint(20, 20));

   // m_cb->SetValue(false);

//    wxImageHandler * jpegLoader = new wxJPEGHandler();
//    wxImage::AddHandler(jpegLoader);

//    bmp.LoadFile("vo.jpg", wxBITMAP_TYPE_JPEG); 

/*
    Connect(ID_CHECKBOX, wxEVT_COMMAND_CHECKBOX_CLICKED, 
          wxCommandEventHandler(CheckBox::OnToggle));
  Centre();
*/

/*
wxMenu *menuFile = new wxMenu;

    menuFile->Append( ID_About, _("&About...") );
    menuFile->AppendSeparator();
    menuFile->Append( ID_Quit, _("E&xit") );

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, _("&File") );

    SetMenuBar( menuBar );
*/

    CreateStatusBar();
    SetStatusText( _("openSMILE GUI plugin.") );
}
/*
void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}
void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox( _("This is a wxWidgets Hello world sample"),
                  _("About Hello World"), 
                  wxOK | wxICON_INFORMATION, this );
}
*/
//IMPLEMENT_APP(MyApp)





//-----

cClassifierResultGUI::cClassifierResultGUI(const char *_name) :
  cSmileComponent(_name)
{
  // ...
}

void cClassifierResultGUI::myFetchConfig()
{
  //cSmileComponent::myFetchConfig();
  
  nClasses = getArraySize("classes");
  nRegression = getArraySize("regression");
  int nImages = getArraySize("images");

  if (nClasses > 0) {
    int i;
    className = (const char**)calloc(1,sizeof(const char*)*nClasses);
    imageFile = (const char**)calloc(1,sizeof(const char*)*nClasses);
    for (i=0; i<nClasses; i++) {
      className[i] = getStr_f(myvprint("classes[%i]",i));
      if (i<nImages) 
        imageFile[i] = getStr_f(myvprint("images[%i]",i));
    }
  }
  if (nRegression > 0) {
    regressionName = (const char**)calloc(1,sizeof(const char*)*nRegression);
    int i;
    for (i=0; i<nRegression; i++) {
      regressionName[i] = getStr_f(myvprint("regression[%i]",i));
    }
  }
}


/*
int cClassifierResultGUI::myConfigureInstance()
{
  return  cDataSink::myConfigureInstance();
  
}
*/


SMILE_THREAD_RETVAL wxClsThreadRunner(void *_data)
{
  if (_data != NULL) {
    cClassifierResultGUI *obj = (cClassifierResultGUI*)_data;
    smileMutexLock(obj->wxMtx);
    obj->pApp = new ClsResultApp(); 
    obj->pApp->smileObj = obj;
    wxApp::SetInstance(obj->pApp);
    smileMutexUnlock(obj->wxMtx);
    wxEntry(0, NULL);
    obj->pApp = NULL;
//    obj->terminate = 1;
    printf("wxwin terminate.\n");
  }
  SMILE_THREAD_RET;
}


int cClassifierResultGUI::myFinaliseInstance()
{
  // FIRST call cDataSink myFinaliseInstance, this will finalise our dataWriter...
  //int ret = cSmileComponent::myFinaliseInstance();

  /* if that was SUCCESSFUL (i.e. ret == 1), then do some stuff like
     loading models or opening output files: */

  
  //fo = new openSmilePluginGUI::TestForm();
  //IMPLEMENT_APP_NO_MAIN(appname)
  int ret = 1;
  smileMutexCreate(wxMtx);

  if (!(int)smileThreadCreate(guiThr, wxClsThreadRunner, this)) {
    SMILE_IERR(1,"error creating GUI thread!!");
  }

  return ret;
}

// search for a class by its name and get the local index
int cClassifierResultGUI::getClassIndex(const char *name)
{
  int i;
  if (className == NULL) return -1;
  if (name == NULL) return -1;
  for (i=0; i<nClasses; i++) {
    if (!strcmp(name,className[i])) { return i; }
  }
  return -1;
}

// search for a class by its name and get the local index
int cClassifierResultGUI::getRegressionIndex(const char *name)
{
  int i;
  if (regressionName == NULL) return -1;
  if (name == NULL) return -1;
  for (i=0; i<nRegression; i++) {
    if (!strcmp(name,regressionName[i])) { return i; }
  }
  return -1;
}

int cClassifierResultGUI::processComponentMessage( cComponentMessage *_msg )
{
  // TODO: classificationResult's' message for multiple outputs!
  smileMutexLock(wxMtx);
  if (pApp == NULL) return 0;
  smileMutexUnlock(wxMtx);

  if (isMessageType(_msg,"classificationResult")) {  
    // determine origin by message's user-defined name, which can be set in the config file
    int nCl = _msg->intData[0]; // number of classes in message (for class confidences)

    SMILE_IMSG(3,"received 'classificationResult' message (nCl = %i)",nCl);


    //int cl = getClassIndex((const char*)_msg->custData2);
    //int rg = -1;
    //if (nCl <= 1) { 

    int rg = getRegressionIndex((const char*)(_msg->custData2));
    if (rg < 0) {
      //SMILE_IERR(3,"unknown classification result message '%s', name not found in local gui config",(const char*)(_msg->custData2));
      //return 0;
      classificationEvent *c = new classificationEvent(nClasses,(int)(_msg->floatData[0]),_msg->msgname,0);
      int i;
      for (i=0; i<MIN(nCl,nClasses); i++) {
        if (_msg->custData != NULL) {
          c->confidences[i] = ((double*)(_msg->custData))[i];
        } else {
          c->confidences[i] = 0.0;
        }
      }

      wxCommandEvent myevent(wxEVT_COMMAND_TEXT_UPDATED, RESULT_UPDATE_ID);
      myevent.SetClientData(c);
      pApp->mframe->GetEventHandler()->AddPendingEvent( myevent );
    } else {
      classificationEvent *c = new classificationEvent(0,0,(const char*)(_msg->custData2),nRegression);
      c->regUpdate = rg;
      c->regressionValue[rg] = _msg->floatData[0];
      printf ("regression rg=%i val=%f\n",rg,_msg->floatData[0]);
      wxCommandEvent myevent(wxEVT_COMMAND_TEXT_UPDATED, RESULT_UPDATE_ID);
      myevent.SetClientData(c);
      pApp->mframe->GetEventHandler()->AddPendingEvent( myevent );
    }


    
    return 1;  // message was processed
  }

  return 0;
}

eTickResult cClassifierResultGUI::myTick(long long t)
{
  return TICK_INACTIVE;

}
#if 0
  SMILE_IDBG(4,"tick # %i, reading value vector:",t);
  cVector *vec= reader->getFrameRel(lag);  //new cVector(nValues+1);
  if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;
  //else reader->nextFrame();

  long vi = vec->tmeta->vIdx;
  double tm = vec->tmeta->time;

  // update VA status in GUI....
//  pApp->AddPendingEvent();
// notify the main thread
   
  if ((int)(vec->data[0]) != old) {
   old = (int)(vec->data[0]);
   wxCommandEvent myevent(wxEVT_COMMAND_TEXT_UPDATED, VAD_UPDATE_ID);
   myevent.SetInt((int)(vec->data[0]));  // pass some data along the event, a number in this case
   pApp->mframe->GetEventHandler()->AddPendingEvent( myevent );
  }

/*  
  // now print the vector:
  int i;
  for (i=0; i<vec->N; i++) {
    //SMILE_PRINT("  (a=%i vi=%i, tm=%fs) %s.%s = %f",reader->getCurR(),vi,tm,reader->getLevelName().c_str(),vec->name(i).c_str(),vec->data[i]);
    printf("  %s.%s = %f\n",reader->getLevelName().c_str(),vec->name(i).c_str(),vec->data[i]);
  }
  
  if (fHandle != NULL) {
    for (i=0; i<vec->N; i++) {
      fprintf(fHandle, "%s = %f\n",vec->name(i).c_str(),vec->data[i]);
    }
  }
*/
  return TICK_SUCCESS;
}
#endif

cClassifierResultGUI::~cClassifierResultGUI()
{
  if (className != NULL) free(className);
  if (regressionName != NULL) free(regressionName);
  if (imageFile != NULL) free(imageFile);
}

