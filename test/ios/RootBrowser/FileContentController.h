//
//  FileContentController.h
//  root_browser
//
//  Created by Timur Pocheptsov on 8/19/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

namespace ROOT_iOS {

class FileContainer;

}

@class ROOTObjectController;
@class SlideshowController;
@class ObjectShortcut;

@interface FileContentController : UIViewController {
   SlideshowController *slideshowController;
   ROOTObjectController *objectController;
   
   NSMutableArray *objectShortcuts;
   
   ROOT_iOS::FileContainer *fileContainer;
}

- (void) activateForFile : (ROOT_iOS::FileContainer *)container;
- (void) selectObjectFromFile : (ObjectShortcut *)obj;

@end
