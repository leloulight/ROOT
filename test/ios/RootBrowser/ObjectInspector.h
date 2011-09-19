#import <UIKit/UIKit.h>

#import "ObjectInspectorComponent.h"

@class EditorView;

class TObject;

namespace ROOT_IOSObjectInspector {

enum {


//Just indices.
kAttLine = 0,
kAttFill = 1,
kAttPad = 2,
kAttAxis  = 3,
//Add the new one here.
kNOfInspectors //
};

}


@interface ObjectInspector : UIViewController <ObjectInspectorComponent> {
   UIViewController <ObjectInspectorComponent> *activeEditors[ROOT_IOSObjectInspector::kNOfInspectors];
   UIViewController <ObjectInspectorComponent> *cachedEditors[ROOT_IOSObjectInspector::kNOfInspectors];

   unsigned nActiveEditors;
   
   TObject *object;
   
   EditorView *editorView;
}

- (void) setROOTObjectController : (ROOTObjectController *)c;
- (void) setROOTObject : (TObject *)o;
- (void) resetInspector;

- (EditorView *) getEditorView;

@end
