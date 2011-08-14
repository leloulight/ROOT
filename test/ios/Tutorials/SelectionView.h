//
//  SelectionView.h
//  Tutorials
//
//  Created by Timur Pocheptsov on 7/22/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

namespace ROOT_iOS {

class Painter;
class Pad;

}

@class PadView;

@interface SelectionView : UIView {
   BOOL showRotation;
   int ev;
   int px;
   int py;
   ROOT_iOS::Pad *pad;

   PadView *view;
}

- (void) setShowRotation : (BOOL) show;
- (void) setEvent : (int) ev atX : (int) x andY : (int) y;
- (void) setPad : (ROOT_iOS::Pad *)pad;

@end
