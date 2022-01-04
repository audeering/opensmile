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


#include <simpleVisualiserGUI.hpp>

#define MODULE "cSimpleVisualiserGUI"


SMILECOMPONENT_STATICS(cSimpleVisualiserGUI)

SMILECOMPONENT_REGCOMP(cSimpleVisualiserGUI)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CSIMPLEVISUALISERGUI;
  sdescription = COMPONENT_DESCRIPTION_CSIMPLEVISUALISERGUI;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    //ct->setField("filename","The name of a text file to dump values to (this file will be overwritten, if it exists)",(const char *)NULL);
    //ct->setField("lag","Output data <lag> frames behind",0,0,0);
    ct->setField("action","type of visualisation action to perform: moving2dplot, movingMatplot","moving2dplot");
    ct->setField("inputs","array that selects the inputs to plot (by name!) (see inputsIdx for listing them by index in the input vector)",(const char*)NULL,ARRAY_TYPE);
    ct->setField("inputsIdx","array that selects the inputs to plot by index in the input vector",0,ARRAY_TYPE);
    ct->setField("fullVectorAsInput","don't select individual elements, use full input vector for plotting (this is especially useful for matrix plots)",0);
    ct->setField("inputScale","scale factor for each input element",1.0,ARRAY_TYPE);
    ct->setField("inputscaleFullinput","scale factor for all fields when fullVectorAsInput=1.",1.0);
    ct->setField("inputoffsetFullinput","additive offset for all fields when fullVectorAsInput=1.",0.0);
    ct->setField("inputOffset","additive offset for each input element (the offset is applied before the scaling:  y=(x+offset)*scale ",0.0,ARRAY_TYPE);
    ct->setField("inputColours","array that configures the plot colours for each input element.",(const char*)NULL,ARRAY_TYPE);
    ct->setField("matMultiplier","number of times to repeat each line in a mat-plot, use this to zoom in on the y axis",1);
  )

  SMILECOMPONENT_MAKEINFO(cSimpleVisualiserGUI);
}


//SMILECOMPONENT_CREATE(cSimpleVisualiserGUI)
cSmileComponent * cSimpleVisualiserGUI::create(const char*_instname) { 
  cSmileComponent *c = new cSimpleVisualiserGUI(_instname); 
  if (c!=NULL) c->setComponentInfo(COMPONENT_NAME_CSIMPLEVISUALISERGUI,COMPONENT_DESCRIPTION_CSIMPLEVISUALISERGUI); 
  return c; 
}

//--- wx ---




wxRtplotPanel::wxRtplotPanel(wxFrame* parent) :
wxPanel(parent),
lastT(0),curT(0),
//curval(0),lastval(0)
paintEventCur(NULL),paintEventLast(NULL)
{
    
}
 
/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */
 
void wxRtplotPanel::paintEvent(wxPaintEvent & evt)
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
void wxRtplotPanel::paintNow()
{
    // depending on your system you may need to look at double-buffered dcs
    wxClientDC dc(this);
    render(dc);
}
 
/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void wxRtplotPanel::render(wxDC&  dc)
{
  smileMutexLock(wxMtx);
  int action;
  if (showImg && paintEventCur != NULL) {
    action = paintEventCur->action;
  } else {
    return;
  }
  smileMutexUnlock(wxMtx);
  switch (action) {
    case VIS_ACT_MOVINGMATPLOT:
      renderMovingMatPlot(dc);
      break;
    case VIS_ACT_MOVING2DPLOT:
    default:
      renderMoving2dPlot(dc);
  }
}
  
void wxRtplotPanel::renderMoving2dPlot(wxDC&  dc)
{
  smileMutexLock(wxMtx);

  if (showImg && paintEventCur != NULL) {

    ////// draw the plot from the plot data
   if (curT == 0) dc.Clear();

   int coord0x = 50;
   int coord0y = 350;
   int coordWidth = 600;
   int coordHeight = 300;

   // background
   dc.FloodFill(1,1,wxColour("white"),wxFLOOD_BORDER);

    // axes
    dc.DrawLine(coord0x,coord0y-coordHeight,coord0x,coord0y+10);
    dc.DrawLine(coord0x-10,coord0y,coord0x+coordWidth+20,coord0y);
      // axes arrows
    dc.DrawLine(coord0x,coord0y-coordHeight,coord0x+10,coord0y-coordHeight+10);
    dc.DrawLine(coord0x,coord0y-coordHeight,coord0x-10,coord0y-coordHeight+10);
    if (paintEventCur->names != NULL && paintEventCur->names[0] != NULL) {
      dc.DrawText(paintEventCur->names[0],coord0x-20,coord0y-coordHeight-30);
    } else {
      dc.DrawText("value",coord0x-20,coord0y-coordHeight-30);
    }
    dc.DrawLine(coord0x+coordWidth+20,coord0y,coord0x+coordWidth+10,coord0y-10);
    dc.DrawLine(coord0x+coordWidth+20,coord0y,coord0x+coordWidth+10,coord0y+10);
    dc.DrawText("t / sec.",coord0x+coordWidth+15,coord0y+20);
      
    // axes ticks
    int i;
    for (i=0; i<=(coordWidth / 100); i++) { // t axis
      dc.DrawLine(coord0x+i*100,coord0y-5,coord0x+i*100,coord0y+5);
      char * x = myvprint("%i",i);
      if (x!=NULL) {
        dc.DrawText(x,coord0x+i*100-5,coord0y+20);
        free(x);
      }
    }
    for (i=0; i<(coordHeight / 100); i++) { // y axis
      dc.DrawLine(coord0x-5,coord0y-i*100,coord0x+5,coord0y-i*100);
      
      FLOAT_DMEM tv = (FLOAT_DMEM)i*paintEventCur->scale0 + paintEventCur->offset0;
      char * x;
      if ((tv < 1.0)&&(tv>-1.0)) {
        if (tv == 0.0) x = myvprint("0.0");
        else x = myvprint("%.3f",tv);
      } else {
        if ((tv < 100.0)&&(tv >-100.0)) {
          x = myvprint("%.2f",tv);
        } else {
          x = myvprint("%i",(int)tv);
        }
      }
      if (x!=NULL) {
        dc.DrawText(x,5,coord0y-i*100);
        free(x);
      }
      
    }

    // TODO: save the axes and background as bitmap, and only plot bitmap


    // data curve(s)
    //int  i;
    for (i=0; i<paintEventCur->nVals; i++) {
      int lastval = 0;

      if (paintEventLast != NULL && paintEventLast->val != NULL) {
        if (i<paintEventLast->nVals)
          lastval = paintEventLast->val[i];
      }
      if (paintEventCur->colours != NULL && paintEventCur->colours[i] != NULL) {
        // there is some kind of bug here... either wrong indicies, or uncaught exceptions
        dc.SetPen(wxPen(wxColour(paintEventCur->colours[i]),2));
      } else {
        dc.SetPen(wxPen(wxColour("blue"),2));
      }
      if (curT != 0) {
        dc.DrawLine(coord0x+lastT,coord0y-lastval,coord0x+curT,coord0y-(paintEventCur->val[i]));
      }
  
    }

    if (paintEventLast != NULL) { delete paintEventLast; }
    paintEventLast = paintEventCur;

    lastT = curT;
    curT = (curT+1)%coordWidth;
    
    // TODO: save plot as bitmap for OnPaint event

  // wxBitmap b = dc.GetAsBitmap();


  } else {
    dc.Clear();
  }

  smileMutexUnlock(wxMtx);
}


void wxRtplotPanel::renderMovingMatPlot(wxDC&  dc)
{
  smileMutexLock(wxMtx);

  if (showImg && paintEventCur != NULL) {

    ////// draw the plot from the plot data
   if (curT == 0) dc.Clear();

   int coord0x = 50;
   int coord0y = 350;
   int coordWidth = 600;
   int coordHeight = 300;

   // background
   dc.FloodFill(1,1,wxColour("white"),wxFLOOD_BORDER);

    // axes
    dc.DrawLine(coord0x,coord0y-coordHeight,coord0x,coord0y+10);
    dc.DrawLine(coord0x-10,coord0y,coord0x+coordWidth+20,coord0y);
      // axes arrows
    dc.DrawLine(coord0x,coord0y-coordHeight,coord0x+10,coord0y-coordHeight+10);
    dc.DrawLine(coord0x,coord0y-coordHeight,coord0x-10,coord0y-coordHeight+10);
    if (paintEventCur->names != NULL && paintEventCur->names[0] != NULL) {
      dc.DrawText(paintEventCur->names[0],coord0x-20,coord0y-coordHeight-30);
    } else {
      dc.DrawText("value",coord0x-20,coord0y-coordHeight-30);
    }
    dc.DrawLine(coord0x+coordWidth+20,coord0y,coord0x+coordWidth+10,coord0y-10);
    dc.DrawLine(coord0x+coordWidth+20,coord0y,coord0x+coordWidth+10,coord0y+10);
    dc.DrawText("t / sec.",coord0x+coordWidth+15,coord0y+20);
      
    // axes ticks
    int i;
    for (i=0; i<=(coordWidth / 100); i++) { // t axis
      dc.DrawLine(coord0x+i*100,coord0y-5,coord0x+i*100,coord0y+5);
      char * x = myvprint("%i",i);
      if (x!=NULL) {
        dc.DrawText(x,coord0x+i*100-5,coord0y+20);
        free(x);
      }
    }
    for (i=0; i<(coordHeight / 100); i++) { // y axis
      dc.DrawLine(coord0x-5,coord0y-i*100,coord0x+5,coord0y-i*100);
      
      FLOAT_DMEM tv = (FLOAT_DMEM)i*paintEventCur->scale0 + paintEventCur->offset0;
      char * x;
      if ((tv < 1.0)&&(tv>-1.0)) {
        if (tv == 0.0) x = myvprint("0.0");
        else x = myvprint("%.3f",tv);
      } else {
        if ((tv < 100.0)&&(tv >-100.0)) {
          x = myvprint("%.2f",tv);
        } else {
          x = myvprint("%i",(int)tv);
        }
      }
      if (x!=NULL) {
        dc.DrawText(x,5,coord0y-i*100);
        free(x);
      }
      
    }

    // TODO: save the axes and background as bitmap, and only plot bitmap

    int mu = paintEventCur->matMultiplier;

    // data curve(s)
    //int  i;
    for (i=0; i<MIN(paintEventCur->nVals,coordHeight); i++) {
      int v = paintEventCur->val[i];
      if (v > 255) v = 255; 
      if (v < 0) v = 0;
      dc.SetPen(wxPen(wxColour(v,v,v)));      
      if (curT != 0) {
        if (mu > 1) {
          dc.DrawLine(coord0x+curT,coord0y-i*mu-1 , coord0x+curT,coord0y-(i+1)*mu-2 );
        } else {
          dc.DrawPoint(coord0x+curT,coord0y-i-1);
        }
      }
    }

    if (paintEventLast != NULL) { delete paintEventLast; }
    paintEventLast = paintEventCur;

    lastT = curT;
    curT = (curT+1)%coordWidth;
    
    // TODO: save plot as bitmap for OnPaint event

  // wxBitmap b = dc.GetAsBitmap();


  } else {
    dc.Clear();
  }

  smileMutexUnlock(wxMtx);
}

enum
{
    ID_Quit = 1,
    ID_About,
};

// catch the event from the thread
BEGIN_EVENT_TABLE(visFrame, wxFrame)
EVT_COMMAND  (PLOT_UPDATE_ID, wxEVT_COMMAND_TEXT_UPDATED, visFrame::onPlotUpdate)
END_EVENT_TABLE()


BEGIN_EVENT_TABLE(wxRtplotPanel, wxPanel)
// catch paint events
EVT_PAINT(wxRtplotPanel::paintEvent)
END_EVENT_TABLE()


const int ID_CHECKBOX = 100;


bool visApp::OnInit()
{
    // make sure to call this first
    //wxInitAllImageHandlers();
    vframe = new visFrame( _("TUM openSMILE visualiser demo window"), wxPoint(50, 50), wxSize(750, 440), wxMtx );
    //vframe->yscale100 = yscale100;
    vframe->Show(true);
    SetTopWindow(vframe);
    return true;
}

visFrame::visFrame(const wxString& title, const wxPoint& pos, const wxSize& size, smileMutex _wxMtx)
       : wxFrame(NULL, wxID_ANY, title, pos, size)
{

    drawPane = new wxRtplotPanel( this );
    drawPane->wxMtx = _wxMtx;
    wxMtx = _wxMtx;

    CreateStatusBar();
    SetStatusText( _("openSMILE visualiser GUI.") );
}

void visFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

//IMPLEMENT_APP(MyApp)





//-----

cSimpleVisualiserGUI::cSimpleVisualiserGUI(const char *_name) :
  cDataSink(_name),
  inputsIdx(NULL), nInputs(0),
  inputScale(NULL), inputOffset(NULL),
  inputNames(NULL), inputColours(NULL),
  pApp(NULL), terminate(0)
{
  // ...
}

void cSimpleVisualiserGUI::myFetchConfig()
{
  cDataSink::myFetchConfig();
  /*
  ct->setField("action","type of visualisation action to perform","movingplot");
    ct->setField("inputs","array that selects the inputs to plot (by name!) (see inputsIdx for listing them by index in the input vector)",(const char*)NULL,ARRAY_TYPE);
    ct->setField("inputsIdx","array 
    */
  const char * actStr = getStr("action");
  if (!strncmp(actStr,"moving2dplot",12)) {
    action = VIS_ACT_MOVING2DPLOT;
  } else if (!strncmp(actStr,"movingMatplot",13)) {
    action = VIS_ACT_MOVINGMATPLOT;
  } else { // default action:
    action = VIS_ACT_MOVING2DPLOT;
  }

  fullinput = getInt("fullVectorAsInput");

  if (!fullinput) {
    int i;
    nInputs = getArraySize("inputs");
    if (nInputs <= 0 ) {
      nInputs = getArraySize("inputsIdx");
      if (nInputs <= 0) {
        SMILE_IERR(1,"either 'inputs' or 'inputsIdx' with at least one element must be set in the config to configure at least on element in the input vector to plot");
        COMP_ERR("aborting.");
      }
      // read indicies  directly from inputsIdx
      inputsIdx = new int[nInputs];
      for (i=0; i<nInputs; i++) {
        inputsIdx[i] = getInt_f(myvprint("inputsIdx[%i]",i));
      }
    } else {
      // read strings and translate to indicies
      // ... this needs to be done in myFinaliseInstance
    }
  }

  if (nInputs > 0) {
    inputScale = new FLOAT_DMEM[nInputs];
    inputOffset = new FLOAT_DMEM[nInputs];
    inputColours = (const char **)calloc(1,sizeof(const char *)*nInputs);
    int i;
    for (i=0; i<nInputs; i++) {
      inputScale[i] = (FLOAT_DMEM)getDouble_f(myvprint("inputScale[%i]",i));
      inputOffset[i] = (FLOAT_DMEM)getDouble_f(myvprint("inputOffset[%i]",i));
      inputColours[i] = getStr_f(myvprint("inputColours[%i]",i));
    }
  }

  matMultiplier=getInt("matMultiplier");
}


/*
int cSimpleVisualiserGUI::myConfigureInstance()
{
  return  cDataSink::myConfigureInstance();
  
}
*/


SMILE_THREAD_RETVAL wxVisAppThreadRunner(void *_data)
{
  if (_data != NULL) {
    cSimpleVisualiserGUI *obj = (cSimpleVisualiserGUI*)_data;
    smileMutexLock(obj->wxMtx);
    obj->pApp = new visApp(); 
    obj->pApp->wxMtx = obj->wxMtx;
    //obj->pApp->yscale100 = obj->yscale100;
    wxApp::SetInstance(obj->pApp);
    // we need a mutex / flag here, to notify the main app that our wxApp was created!
    smileMutexUnlock(obj->wxMtx);
    wxEntry(0, NULL);
    smileMutexLock(obj->wxMtx);
    //delete (obj->pApp);
    obj->pApp = NULL;
    obj->terminate = 1;
    printf("wxwin terminate.\n");
    smileMutexUnlock(obj->wxMtx);
  }
  SMILE_THREAD_RET;
}


// TODO: use dataProcessor::findElement, once it is comitted to the openSMILE trunk
int cSimpleVisualiserGUI::getElidxFromName(const char *_name) 
{
  const FrameMetaInfo * fmeta = reader->getFrameMetaInfo();
  int ri;
  long elIdx=-1;
  long idx = fmeta->findFieldByPartialName( _name , &ri, NULL );
  if (idx < 0) {
    SMILE_IWRN(2,"Requested input element '*%s*' not found, check your config!",_name);
  } else {
    elIdx = fmeta->fieldToElementIdx( idx ) + ri;
  }
  return elIdx;
}

int cSimpleVisualiserGUI::myFinaliseInstance()
{
  // FIRST call cDataSink myFinaliseInstance, this will finalise our dataWriter...
  int ret = cDataSink::myFinaliseInstance();

  int i;
  if (ret && fullinput) {
    nInputs = reader->getLevelN();
    if (nInputs > 0) {
      inputNames = (const char**)calloc(1,sizeof(const char *)*nInputs);
      inputScale = new FLOAT_DMEM[nInputs];
      inputOffset = new FLOAT_DMEM[nInputs];
      inputsIdx = new int[nInputs];
      FLOAT_DMEM scaleAll = getDouble("inputscaleFullinput");
      FLOAT_DMEM offsetAll = getDouble("inputoffsetFullinput");
      for (i=0; i<nInputs; i++) {
        inputsIdx[i] = i;
        inputNames[i] = reader->getElementName(inputsIdx[i]);
        inputScale[i] = scaleAll;
        inputOffset[i] = offsetAll;
      }
    }
  } else
  if (ret && nInputs > 0 ) {
    inputNames = (const char**)calloc(1,sizeof(const char *)*nInputs);
    if (inputsIdx == NULL) {

      inputsIdx = new int[nInputs];
      for (i=0; i<nInputs; i++) {
        const char * tmp = getStr_f(myvprint("inputs[%i]",i));
        inputNames[i] = tmp; 
        if (tmp != NULL) {
          long idx = getElidxFromName(tmp);
          inputsIdx[i] = idx;
        } else {
          inputsIdx[i] = -1;
        }
      }

    } else {
      for (i=0; i<nInputs; i++) {
        inputNames[i] = reader->getElementName(inputsIdx[i]);
      }
    }
  }

  smileMutexCreate(wxMtx);

  //yscale100 = 100.0 / inputScale[0];

  //fo = new openSmilePluginGUI::TestForm();
  //IMPLEMENT_APP_NO_MAIN(appname)
  if (!(int)smileThreadCreate(guiThr, wxVisAppThreadRunner, this)) {
    SMILE_IERR(1,"error creating GUI thread!!");
  }

  return ret;
}


eTickResult cSimpleVisualiserGUI::myTick(long long t)
{
  SMILE_IDBG(4,"tick # %i, reading value vector:",t);
  cVector *vec = NULL;
  if (!terminate) vec= reader->getNextFrame();  
  if (vec == NULL) return TICK_SOURCE_NOT_AVAIL;
  //else reader->nextFrame();

  long vi = vec->tmeta->vIdx;
  double tm = vec->tmeta->time;

  smileMutexLock(wxMtx);
  if (pApp == NULL) {
    smileMutexUnlock(wxMtx);
    if (terminate) return TICK_INACTIVE;
    else return TICK_SUCCESS;
  }
  smileMutexUnlock(wxMtx);

  // update VA status in GUI....
//  pApp->AddPendingEvent();
// notify the main thread
  
  int nr = 0;
  paintDataEvent *pe = new paintDataEvent(nInputs);

  for (nr = 0; nr<nInputs; nr++) {

    long idx = inputsIdx[nr];
    FLOAT_DMEM data = 0.0;
    if (idx >= 0) data = vec->data[idx];
    pe->val[nr] = (int)((data+inputOffset[nr])*inputScale[nr]);

  }
  pe->scale0 = 100.0 / inputScale[0];
  pe->offset0 = inputOffset[nr]*inputScale[0];
  pe->names = inputNames;
  pe->colours = inputColours;
  pe->action = action;
  pe->matMultiplier = matMultiplier;

  //smileMutexLock(wxMtx);
  wxCommandEvent myevent(wxEVT_COMMAND_TEXT_UPDATED, PLOT_UPDATE_ID);
//  myevent.SetInt((int)((data+inputOffset[nr])*inputScale[nr]));  // pass some data along the event, a number in this case
//  myevent.SetString(vec->fmeta->getName(idx)); // TODO: this name is not present when using inputsIdx in the config...
  myevent.SetClientData(pe);
  pApp->vframe->GetEventHandler()->AddPendingEvent( myevent );
  //smileMutexUnlock(wxMtx);

  return TICK_SUCCESS;
}


cSimpleVisualiserGUI::~cSimpleVisualiserGUI()
{
  if (inputsIdx != NULL) {
    delete[] inputsIdx;
  }
  if (inputScale != NULL) {
    delete[] inputScale;
  }
  if (inputOffset != NULL) {
    delete[] inputOffset;
  }
  if (inputColours != NULL) {
    free(inputColours);
  }
  if (inputNames != NULL) {
    free(inputNames);
  }

  // TODO: send event to make wxApp terminate
  smileThreadJoin(guiThr);

  smileMutexDestroy(wxMtx);

}

