////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
///  This is beta software and is subject to changes without notice.
///
///  Do not distribute.
///
///  Copyright (c) 2009 Aurora Feint Inc. All rights reserved.
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#import <UIKit/UIKit.h>

@interface OFRootController : UIViewController
{
@package
	BOOL isFullScreen;
}

@property (nonatomic, readonly) BOOL isFullScreen;

@end