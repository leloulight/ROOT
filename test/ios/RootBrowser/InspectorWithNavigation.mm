#import "InspectorWithNavigation.h"

@implementation InspectorWithNavigation

//_________________________________________________________________
- (void)didReceiveMemoryWarning
{
   // Releases the view if it doesn't have a superview.
   [super didReceiveMemoryWarning];
   // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

/*
// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView
{
}
*/

/*
// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
    [super viewDidLoad];
}
*/

//_________________________________________________________________
- (void)viewDidUnload
{
   [super viewDidUnload];
   // Release any retained subviews of the main view.
   // e.g. self.myOutlet = nil;
}

//_________________________________________________________________
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
   // Return YES for supported orientations
	return YES;
}

//_________________________________________________________________
- (void) setROOTObjectController : (ROOTObjectController *)c
{
   UIViewController<ObjectInspectorComponent> *rootController = (UIViewController<ObjectInspectorComponent> *)[self.viewControllers objectAtIndex : 0];
   [rootController setROOTObjectController : c];
}

//_________________________________________________________________
- (void) setROOTObject : (TObject *)obj
{
   UIViewController<ObjectInspectorComponent> *rootController = (UIViewController<ObjectInspectorComponent> *)[self.viewControllers objectAtIndex : 0];
   [rootController setROOTObject : obj];
}

//_________________________________________________________________
- (NSString *) getComponentName
{
   UIViewController<ObjectInspectorComponent> *rootController = (UIViewController<ObjectInspectorComponent> *)[self.viewControllers objectAtIndex : 0];
   return [rootController getComponentName];
}

//_________________________________________________________________
- (void) resetInspector
{
   //Pop all controllers from a stack except a top level root controller.
   while ([self.viewControllers count] > 1)
      [self popViewControllerAnimated : NO];
}

@end
