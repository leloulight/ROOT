#ifndef ROOT_TGLViewerEditor
#define ROOT_TGLViewerEditor

#include <memory>

#ifndef ROOT_TGedFrame
#include "TGedFrame.h"
#endif

#ifndef ROOT_TGLUtil
#include "TGLUtil.h"
#endif

class TGCheckButton;
class TGNumberEntry;
class TGButtonGroup;
class TGroupFrame;
class TGRadioButton;
class TGColorSelect;
class TGComboBox;
class TGButton;
class TGLViewer;
class TGTab;

class TGLLightSetSubEditor;
class TGLClipSetSubEditor;

class TGLViewerEditor : public TGedFrame
{
private:
   //Pointers to manipulate with tabs
   TGCompositeFrame *fGuidesFrame;
   TGCompositeFrame *fClipFrame;

   TGLLightSetSubEditor *fLightSet;

   TGColorSelect    *fClearColor;
   TGCheckButton    *fIgnoreSizesOnUpdate;
   TGCheckButton    *fResetCamerasOnUpdate;
   TGCheckButton    *fResetCameraOnDoubleClick;
   TGTextButton     *fUpdateScene;
   TGTextButton     *fCameraHome;

   //"Guides" tab's controls
   TGCheckButton    *fCameraCenterExt;
   TGTextButton     *fCaptureCenter;
   TGCheckButton    *fDrawCameraCenter;
   TGNumberEntry    *fCameraCenterX;
   TGNumberEntry    *fCameraCenterY;
   TGNumberEntry    *fCameraCenterZ;

   Int_t             fAxesType;
   TGButtonGroup    *fAxesContainer;
   TGRadioButton    *fAxesNone;
   TGRadioButton    *fAxesEdge;
   TGRadioButton    *fAxesOrigin;
   TGCheckButton    *fAxesDepthTest;

   TGGroupFrame     *fRefContainer;
   TGCheckButton    *fReferenceOn;
   TGNumberEntry    *fReferencePosX;
   TGNumberEntry    *fReferencePosY;
   TGNumberEntry    *fReferencePosZ;

   TGGroupFrame     *fCamContainer;
   TGComboBox*       fCamMode;
   TGCheckButton*    fCamMarkupOn;

   TGLClipSetSubEditor *fClipSet;

   //Model
   TGLViewer        *fViewer;
   Bool_t	     fIsInPad;

   void ConnectSignals2Slots();

   TGLViewerEditor(const TGLViewerEditor &);
   TGLViewerEditor &operator = (const TGLViewerEditor &);

   void CreateStyleTab();
   void CreateGuidesTab();
   void CreateClippingTab();

   void UpdateReferencePosState();

public:
   TGLViewerEditor(const TGWindow *p=0, Int_t width=140, Int_t height=30,
                   UInt_t options=kChildFrame, Pixel_t back=GetDefaultFrameBackground());
   ~TGLViewerEditor();

   virtual void ViewerRedraw();

   virtual void SetModel(TObject* obj);

   void SetGuides();
   void DoClearColor(Pixel_t color);
   void DoIgnoreSizesOnUpdate();
   void DoResetCamerasOnUpdate();
   void DoResetCameraOnDoubleClick();
   void DoUpdateScene();
   void DoCameraHome();
   void DoCameraCenterExt();
   void DoCaptureCenter();
   void DoDrawCameraCenter();
   void UpdateCameraCenter();
   //Axis manipulation
   void UpdateViewerAxes(Int_t id);
   void UpdateViewerReference();
   void DoCameraMarkup();

   void DetachFromPad(){fIsInPad = kFALSE;}

   static TGNumberEntry* MakeLabeledNEntry(TGCompositeFrame* p, const char* name, Int_t labelw,Int_t nd=7, Int_t s=5);

   ClassDef(TGLViewerEditor, 0) //GUI for editing TGLViewer attributes
};

#endif
