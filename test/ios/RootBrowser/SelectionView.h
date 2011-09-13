//
//  SelectionView.h
//  root_browser
//
//  Created by Timur Pocheptsov on 8/31/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>

namespace ROOT_iOS {

class Pad;

}

@interface SelectionView : UIView {
   ROOT_iOS::Pad *pad;
   
   BOOL panActive;
   CGPoint panStart;
   CGPoint currentPanPoint;
   BOOL verticalDirection;
}

@property (nonatomic, assign) BOOL panActive;
@property (nonatomic, assign) CGPoint panStart;
@property (nonatomic, assign) CGPoint currentPanPoint;
@property (nonatomic, assign) BOOL verticalDirection;

- (id)initWithFrame : (CGRect)frame withPad : (ROOT_iOS::Pad *) p;

@end
