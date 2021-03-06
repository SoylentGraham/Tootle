/*
 *  TFacebookSession.mm
 *  TootleSocial
 *
 *  Created by Duane Bradbury on 05/06/2009.
 *  Copyright 2009 Tootle. All rights reserved.
 *
 */

/*
 
// [24/06/10] DB - TEMP REMOVED 
 
#import "TFacebookSession.h"
#import "FBConnect/FBConnect.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// This application will not work until you enter your Facebook application's API key here:

static NSString* kApiKey = @"fcdff620ddcfef5db1c4529a43c50f7b";

// Enter either your API secret or a callback URL (as described in documentation):
static NSString* kApiSecret = @"2c248a981de982d91690d68b6fa0e7b8"; // @"<YOUR SECRET KEY>";
static NSString* kGetSessionProxy = nil; // @"<YOUR SESSION CALLBACK)>";

///////////////////////////////////////////////////////////////////////////////////////////////////

@implementation SessionViewController

@synthesize label = _label;

///////////////////////////////////////////////////////////////////////////////////////////////////
// NSObject


- (void)createSessionView
{
	UIView *view = [[UIView alloc] initWithFrame:[UIScreen mainScreen].applicationFrame];
	
    [view setAutoresizingMask:UIViewAutoresizingNone];	
    //[view setBackgroundColor: [UIColor redColor]];
    [view setBackgroundColor: [UIColor clearColor]];
	
	
	// Set view to be full screen
	[view setFrame:CGRectMake(0, 0, 320, 480)];

    self.view = view;
	
    [view release];
	
	if (kGetSessionProxy) {
		_session = [[FBSession sessionForApplication:kApiKey getSessionProxy:kGetSessionProxy
											delegate:self] retain];
	} else {
		_session = [[FBSession sessionForApplication:kApiKey secret:kApiSecret delegate:self] retain];
	}
	
}

- (void)dealloc {
	[_session release];
	[super dealloc];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// UIViewController

- (void)loadView
{	
	
	[self createSessionView];	

	//////////////////////////////////////////	
	// Create the 'standard' facebook login button. 
	//
	// NOTE: requires the facebook bundle to be added to 
	// the game project for the facebook button image(s)
	// to appear correctly.
	// See the test app for an example.
	//////////////////////////////////////////	
	_loginButton = [[FBLoginButton alloc] initWithFrame:CGRectMake(58, 25, 258, 56)];
	
	[_loginButton setStyle:FBLoginButtonStyleWide];

	[self.view addSubview:_loginButton];
	//////////////////////////////////////////	


	//////////////////////////////////////////	
	// Add a publish feed button
	// 
	// This button is an iPod specific button and will 
	// call the 'publishFeed' callback routine when clicked on
	//////////////////////////////////////////
	_feedButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
	// set position - x,y,w,h
	[_feedButton setFrame:CGRectMake(20, 100, 260, 56)];	
	[_feedButton setHidden:TRUE];

	[_feedButton setTitle:@"Publish Feed" forState:UIControlStateNormal];
	
	SEL feedSelector = @selector(publishFeed:);
	[_feedButton addTarget:nil action:feedSelector forControlEvents:UIControlEventTouchUpInside];
	[self.view addSubview:_feedButton];
	
	//////////////////////////////////////////
	// Add a get permission button
	//
	// This button is an iPod specific button and will 
	// call the 'askPermission' callback routine when clicked on
	//////////////////////////////////////////

	_permissionButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];	
	
	// set position - x,y,w,h
	[_permissionButton setFrame:CGRectMake(20, 185, 260, 56)];
	[_permissionButton setHidden:TRUE];

	[_permissionButton setTitle:@"Get permission" forState:UIControlStateNormal];
	
	SEL permissionSelector = @selector(askPermission:);
	[_permissionButton addTarget:nil action:permissionSelector forControlEvents:UIControlEventTouchUpInside];
	[self.view addSubview:_permissionButton];

	//////////////////////////////////////////

}


- (void)viewDidLoad {
	[_session resume];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	return YES;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FBDialogDelegate

- (void)dialog:(FBDialog*)dialog didFailWithError:(NSError*)error {
	_label.text = [NSString stringWithFormat:@"Error(%d) %@", error.code,
				   error.localizedDescription];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FBSessionDelegate

- (void)session:(FBSession*)session didLogin:(FBUID)uid {
	_permissionButton.hidden = NO;
	_feedButton.hidden = NO;
	
	NSString* fql = [NSString stringWithFormat:
					 @"select uid,name from user where uid == %lld", session.uid];
	
	NSDictionary* params = [NSDictionary dictionaryWithObject:fql forKey:@"query"];
	[[FBRequest requestWithDelegate:self] call:@"facebook.fql.query" params:params];
}

- (void)sessionDidLogout:(FBSession*)session {
	_label.text = @"";
	_permissionButton.hidden = YES;
	_feedButton.hidden = YES;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FBRequestDelegate

- (void)request:(FBRequest*)request didLoad:(id)result {
	NSArray* users = result;
	NSDictionary* user = [users objectAtIndex:0];
	NSString* name = [user objectForKey:@"name"];
	_label.text = [NSString stringWithFormat:@"Logged in as %@", name];
}

- (void)request:(FBRequest*)request didFailWithError:(NSError*)error {
	_label.text = [NSString stringWithFormat:@"Error(%d) %@", error.code,
				   error.localizedDescription];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

- (void)askPermission:(id)target {
	FBPermissionDialog* dialog = [[[FBPermissionDialog alloc] init] autorelease];
	dialog.delegate = self;
	dialog.permission = @"status_update";
	[dialog show];
}

- (void)publishFeed:(id)target {
 FBFeedDialog* dialog = [[[FBFeedDialog alloc] init] autorelease];
	dialog.delegate = self;
	dialog.templateBundleId = 66186134806LL;
	dialog.templateData = @"{\"key1\": \"value1\"}";
	[dialog show];
}

@end
*/