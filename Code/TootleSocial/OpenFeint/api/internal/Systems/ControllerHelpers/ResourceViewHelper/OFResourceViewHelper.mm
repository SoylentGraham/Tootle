////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///  Copyright 2009 Aurora Feint, Inc.
/// 
///  Licensed under the Apache License, Version 2.0 (the "License");
///  you may not use this file except in compliance with the License.
///  You may obtain a copy of the License at
///  
///  	http://www.apache.org/licenses/LICENSE-2.0
///  	
///  Unless required by applicable law or agreed to in writing, software
///  distributed under the License is distributed on an "AS IS" BASIS,
///  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
///  See the License for the specific language governing permissions and
///  limitations under the License.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#import "OFDependencies.h"
#import "OFResourceViewHelper.h"
#import "OFControllerHelpersCommon.h"

@implementation OFResourceViewHelper

- (void)dealloc 
{
	OFSafeRelease(mResource);
	[super dealloc];
}

- (void)changeResource:(OFResource*)resource
{
	[mResource release];
	mResource = [resource retain];
	[self onResourceChanged:mResource];
}

- (void)onResourceChanged:(OFResource*)resource
{
	ASSERT_OVERRIDE_MISSING;
}

@end
