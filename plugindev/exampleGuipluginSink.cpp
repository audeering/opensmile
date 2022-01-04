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


#include <exampleGuipluginSink.hpp>

#define MODULE "cExampleGuipluginSink"


SMILECOMPONENT_STATICS(cExampleGuipluginSink)

SMILECOMPONENT_REGCOMP(cExampleGuipluginSink)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CEXAMPLEGUIPLUGINSINK;
  sdescription = COMPONENT_DESCRIPTION_CEXAMPLEGUIPLUGINSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("filename","The name of a text file to dump values to (this file will be overwritten, if it exists)",(const char *)NULL);
    ct->setField("lag","Output data <lag> frames behind",0,0,0);
  )

  SMILECOMPONENT_MAKEINFO(cExampleGuipluginSink);
}


//SMILECOMPONENT_CREATE(cExampleGuipluginSink)
cSmileComponent * cExampleGuipluginSink::create(const char*_instname) { 
  cSmileComponent *c = new cExampleGuipluginSink(_instname); 
  if (c!=NULL) c->setComponentInfo(COMPONENT_NAME_CEXAMPLEGUIPLUGINSINK,COMPONENT_DESCRIPTION_CEXAMPLEGUIPLUGINSINK); 
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



wxImagePanel::wxImagePanel(wxFrame* parent, wxString file, wxBitmapType format) :
wxPanel(parent)
{
    // load the file... ideally add a check to see if loading was successful
    image.LoadFile(file, format);
}
 
/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */
 
void wxImagePanel::paintEvent(wxPaintEvent & evt)
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
void wxImagePanel::paintNow()
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
void wxImagePanel::render(wxDC&  dc)
{
  if (showImg) {
    dc.DrawBitmap( image, 50, 50, false );
  } else {
    dc.Clear();
  }

}


// catch the event from the thread
BEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_COMMAND  (VAD_UPDATE_ID, wxEVT_COMMAND_TEXT_UPDATED, MyFrame::onVadUpdate)
END_EVENT_TABLE()


BEGIN_EVENT_TABLE(wxImagePanel, wxPanel)

// catch paint events
EVT_PAINT(wxImagePanel::paintEvent)
 
END_EVENT_TABLE()

const int ID_CHECKBOX = 100;


bool MyApp::OnInit()
{
    // make sure to call this first
    wxInitAllImageHandlers();

    

    mframe = new MyFrame( _("TUM openSMILE status window"), wxPoint(50, 50), wxSize(450, 340) );
    mframe->Show(true);
    SetTopWindow(mframe);
    return true;
}

MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
       : wxFrame(NULL, wxID_ANY, title, pos, size)
{
    //wxPanel *panel = new wxPanel(this, wxID_ANY);
    
    
    //wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    drawPane = new wxImagePanel( this, wxT("vo.jpg"), wxBITMAP_TYPE_JPEG);
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
    SetStatusText( _("openSMILE status GUI.") );
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

cExampleGuipluginSink::cExampleGuipluginSink(const char *_name) :
  cDataSink(_name),
  fHandle(NULL)
{
  // ...
}

void cExampleGuipluginSink::myFetchConfig()
{
  cDataSink::myFetchConfig();
  
  filename = getStr("filename");
  SMILE_IDBG(2,"filename = '%s'",filename);
  lag = getInt("lag");
  SMILE_IDBG(2,"lag = %i",lag);
}


/*
int cExampleGuipluginSink::myConfigureInstance()
{
  return  cDataSink::myConfigureInstance();
  
}
*/


SMILE_THREAD_RETVAL wxAppThreadRunner(void *_data)
{
  if (_data != NULL) {
    cExampleGuipluginSink *obj = (cExampleGuipluginSink*)_data;
    obj->pApp = new MyApp(); 
    wxApp::SetInstance(obj->pApp);
    wxEntry(0, NULL);
  }
  SMILE_THREAD_RET;
}


int cExampleGuipluginSink::myFinaliseInstance()
{
  // FIRST call cDataSink myFinaliseInstance, this will finalise our dataWriter...
  int ret = cDataSink::myFinaliseInstance();

  /* if that was SUCCESSFUL (i.e. ret == 1), then do some stuff like
     loading models or opening output files: */

  if ((ret)&&(filename != NULL)) {
    fHandle = fopen(filename,"w");
    if (fHandle == NULL) {
      SMILE_IERR(1,"failed to open file '%s' for writing!",filename);
      COMP_ERR("aborting");
	    //return 0;
    }
  }

  //fo = new openSmilePluginGUI::TestForm();
  //IMPLEMENT_APP_NO_MAIN(appname)
  if (!(int)smileThreadCreate(guiThr, wxAppThreadRunner, this)) {
    SMILE_IERR(1,"error creating GUI thread!!");
  }

  return ret;
}


eTickResult cExampleGuipluginSink::myTick(long long t)
{
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


cExampleGuipluginSink::~cExampleGuipluginSink()
{
  if (fHandle != NULL) {
    fclose(fHandle);
  }
}

