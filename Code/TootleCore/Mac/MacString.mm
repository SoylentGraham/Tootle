/*
 *  MacString.mm
 *  TootleCore
 *
 *  Created by Duane Bradbury on 15/12/2009.
 *  Copyright 2009 Tootle. All rights reserved.
 *
 */


#import "MacString.h"

NSString* TLString::ConvertToUnicharString(const TString& String)
{	
	TArray<unichar> unichararray;
	
	// Build array of unichar characters from the string
	u32 StringLength = String.GetLength();
	const TChar* pData = String.GetData();
	
	for(u32 uIndex = 0; uIndex < StringLength; uIndex++)
	{
		TChar character = pData[uIndex];
		
		// Cast to unichar. Assume top bytes are null
		unichar unicharcharacter = (unichar) character;
		
		// Add inchar to array
		unichararray.Add(unicharcharacter);
	}
	
	// Create an NSString from the unichar character array data
	NSString* pString = [[NSString alloc] initWithCharacters:unichararray.GetData() length:StringLength];
	
	return pString;
}