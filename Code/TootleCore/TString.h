/*------------------------------------------------------
	Our nice flexible engine string type

-------------------------------------------------------*/
#pragma once

#include "TArray.h"
#include "TFixedArray.h"
#include <stdarg.h>	//	for va_list on mac

class TString;

#define TLString_Terminator		((TChar)'\0')

namespace TLString
{
	Bool		IsCharWhitespace(const TChar& Char);
	Bool		IsCharLetter(const TChar& Char);
	Bool		IsCharLowercase(const TChar& Char);
	Bool		IsCharUppercase(const TChar& Char);
	TChar		GetCharLowercase(const TChar& Char);	//	return lowercase version of this char
	Bool		SetCharLowercase(TChar& Char);			//	change this char to be lower case
	Bool		SetCharUppercase(TChar& Char);			//	change this char to be upper case
	s32			GetCharInteger(const TChar& Char);
	s32			GetCharHexInteger(const TChar& Char);
	FORCEINLINE const char*	GetTrueFalse(bool Boolean)			{	return Boolean ? "True" : "False";	}

	template<typename TOCHAR,typename FROMCHAR>
	inline TOCHAR	GetChar(const FROMCHAR& FromChar)	{	return (TOCHAR)FromChar;	}

	//------------------------------------------------------
	//	templated strlen as it's not type specific code...
	//------------------------------------------------------
	template<typename CHARTYPE>
	inline u32	Strlen(const CHARTYPE* pString)
	{
		u32 Len = 0;
		while ( *pString )
		{
			pString++;
			Len++;
		}
		return Len;
	}

	//	templated TypeX append operators. Trying to automate this some more so we don't need to call these at all
	template<typename TYPE2> TString&	AppendType2(TString& String,const TYPE2& Value)	{	String << Value.x << ',' << Value.y;	return String;	}
	template<typename TYPE3> TString&	AppendType3(TString& String,const TYPE3& Value)	{	String << Value.x << ',' << Value.y << ',' << Value.z;	return String;	}
	template<typename TYPE4> TString&	AppendType4(TString& String,const TYPE4& Value)	{	String << Value.x << ',' << Value.y << ',' << Value.z << ',' << Value.w;	return String;	}	
}


//------------------------------------------------------
//	todo: make templated for other string types? or for fixed lengths?
//		currently just dynamic char's
//------------------------------------------------------
class TString
{
public:
	TString()								{	}
	TString(const TString& String)			{	Append( String );	}
	TString(const TChar8* pString,...);											//	formatted constructor
	TString(const TChar16* pString,...);										//	formatted constructor
	virtual ~TString()															{	}

	//	gr: no mutable version of this func. instead, use GetStringArray to access the data, then make sure you SetTerminator after modifying it!
	FORCEINLINE const TChar*	GetData() const								{	return GetStringArray().GetData();	}
	void						GetAnsi(TArray<char>& String,bool IncludeTerminator=true) const;		//	export this string into an ansi string in the form of an array

	//	accessors
	FORCEINLINE u32			GetLength() const								{	u32 Size = GetStringArray().GetSize();	return (Size == 0) ? 0 : Size-1;	}	//	length = size - terminator
	FORCEINLINE Bool		IsEmpty() const									{	return (GetLength() == 0);	}
	FORCEINLINE void		SetLength(u32 NewLength)						{	GetStringArray().SetSize( NewLength );	SetTerminator();	}	//	set the string to a certain size (usually for buffering)
	void					SetAllocSize(u32 AllocLength);					//	allocate buffer for string data so that it doesnt need to alloc more later
	FORCEINLINE u32			GetAllocSize() const							{	return GetStringArray().GetAllocSize();	}
	FORCEINLINE u32			GetMaxAllocSize() const							{	return GetStringArray().GetMaxAllocSize();	}
	FORCEINLINE void		Empty(Bool Dealloc=FALSE)						{	GetStringArray().Empty(Dealloc);	}
	FORCEINLINE void		Set(const TString& String)						{	Empty();	Append( String );	}
	FORCEINLINE void		Set(const TChar8* pString,s32 Length=-1)		{	Empty();	Append( pString, Length );	}
	FORCEINLINE void		Set(const TChar16* pString,s32 Length=-1)		{	Empty();	Append( pString, Length );	}

	FORCEINLINE TString&	Append(const TChar8& Char)						{	TChar c = TLString::GetChar<TChar>(Char);	return Append( &c, 1 );	}
	FORCEINLINE TString&	Append(const TChar16& Char)						{	TChar c = TLString::GetChar<TChar>(Char);	return Append( &c, 1 );	}
	FORCEINLINE TString&	Append(const TString& String,s32 Length=-1)		{	return Append( String.GetData(), (Length<0 || Length > (s32)String.GetLength()) ? String.GetLength() : Length );	}

//	template<class TYPE>
//	TString&				Append(const TYPE& Object)						{	Object.GetString( *this );	return *this;	}
	template<typename CHARTYPE>
	TString&				Append(const CHARTYPE* pString,s32 Length=-1);	//	add string onto the end of the current string
	TString&				Append(const TString& String,u32 From,s32 Length);	//	append part of a string (use -1 as the length to copy everything FROM from)
	TString&				Appendf(const TChar8* pString,...);				//	add string onto the end of the current string
	TString&				Appendf(const TChar16* pString,...);			//	add string onto the end of the current string
	void					InsertAt(u32 Index,const TString& String);		//	insert a string into this string
	Bool					Trim(Bool TrimFront=TRUE);						//	trim whitespace from front and back of string - returns TRUE if changed
	Bool					SetLowercase();									//	change all uppercase characters to lowercase - returns TRUE if changed
	Bool					SetUppercase();									//	change all lowercase characters to uppercase - returns TRUE if changed

	//	string info functions
	FORCEINLINE s32			GetCharIndex(const TChar& Char,u32 From=0) const			{	return GetStringArray().FindIndex( Char, From );	}
	s32						GetCharIndexNonWhitespace(u32 From=0) const;				//	find the next non-whitespace char
	s32						GetCharIndexWhitespace(u32 From=0) const;					//	find the next whitespace char
	FORCEINLINE s32			GetLastCharIndex(const TChar& Char,s32 From=-1) const		{	return GetStringArray().FindIndexReverse( Char, From );	}
	FORCEINLINE Bool		GetCharExists(const TChar& Char) const						{	return GetStringArray().Exists( Char );	}
	Bool					IsEqual(const TString& String,Bool CaseSensitive) const;	//	comparison to string
	Bool					IsLessThan(const TString& String) const;					//	comparison to string
	template<class STRINGTYPE>
	Bool					Split(const TChar& SplitChar,TArray<STRINGTYPE>& StringArray) const;		//	split string by SplitChar into array. if no cases of SplitChar then FALSE is return and no strings are added to the array
	template<class STRINGTYPE>
	Bool					Split(const TArray<TChar>& SplitChars,TArray<STRINGTYPE>& StringArray) const;		//	split string by SplitChar into array. if no cases of SplitChar then FALSE is return and no strings are added to the array
	template<class TYPE_N>
	Bool					GetInteger(TYPE_N/*<int>*/& IntN) const;					//	extract N ints (no more, no less) into a TypeN<int> type - gr: this should support u8,u16 etc, but might throw up type-cast warnings. We could build in the signed/range checking into this routine...
	Bool					GetInteger(s32& Integer,Bool* pIsPositive=NULL) const;		//	turn string into an integer - fails if some non-integer stuff in it (best to trim first where possible). The extra pIsPositive param (if not null) stores the posititivy/negativity of the number. This is needed for -0.XYZ numbers. we still need the sign for floats, but as an int -0 is just 0.
	Bool					GetIntegers(TArray<s32>& Integers) const;					//	read an array of integers from a string
	template<class TYPE_N>
	Bool					GetFloat(TYPE_N/*<float>*/& FloatN) const;					//	extract N floats (no more, no less) into a TypeN<float> type
	Bool					GetFloat(float& Float) const;								//	turn string into a float
	Bool					GetFloats(TArray<float>& Floats) const;						//	get an array of floats from a string (expects just floats)
	Bool					GetHexInteger(u32& Integer) const;							//	turn hexidecimal string into an integer (best to trim first where possible)
	Bool					GetHexBytes(TArray<u8>& ParsedBytes) const;					//	turn hexidecimal string into an array of bytes. so string is expected to be like 0011223344aabbff etc

	//	gr: I *think* it's safe to expose these now...
	virtual TArray<TChar>&			GetStringArray()						{	return m_DataArray;	}
	virtual const TArray<TChar>&	GetStringArray() const					{	return m_DataArray;	}

	TChar&					GetCharAt(u32 Index)							{	return GetStringArray().ElementAt(Index);	}
	const TChar&			GetCharAt(u32 Index) const						{	return GetStringArray().ElementAtConst(Index);	}
	virtual TChar			GetLowercaseCharAt(u32 Index) const				{	return TLString::GetCharLowercase( GetCharAt( Index ) );	}
	TChar					GetCharLast() const;							//	get the last char. if string is empty a terminator is returned. If the string ends with a terminator, it returns the last char before terminator (if any)
	s32						GetCharGetLastIndex() const;					//	get the index of last char. -1 if empty or all terminators
	void					RemoveCharAt(u32 Index,u32 Amount=1)			{	GetStringArray().RemoveAt(Index,Amount);	}

	FORCEINLINE const TChar&	operator[](u32 Index) const					{	return GetCharAt(Index);	}

	FORCEINLINE TString&		operator=(const TChar8* pString)			{	Set( pString );	return *this;	}
	FORCEINLINE TString&		operator=(const TChar16* pString)			{	Set( pString );	return *this;	}
	FORCEINLINE TString&		operator=(const TString& String)			{	Set( String );	return *this;	}
	FORCEINLINE Bool			operator==(const TString& String) const		{	return IsEqual( String, TRUE );	}
	FORCEINLINE Bool			operator==(const TChar8* pString) const		{	return IsEqual( pString, TRUE );	}
	FORCEINLINE Bool			operator==(const TChar16* pString) const	{	return IsEqual( pString, TRUE );	}
	FORCEINLINE Bool			operator!=(const TString& String) const		{	return !IsEqual( String, TRUE );	}
	FORCEINLINE Bool			operator!=(const TChar8* pString) const		{	return !IsEqual( pString, TRUE );	}
	FORCEINLINE Bool			operator!=(const TChar16* pString) const	{	return !IsEqual( pString, TRUE );	}
	FORCEINLINE Bool			operator<(const TString& String) const		{	return IsLessThan( String );	}

protected:
	virtual void				ForceCaseSensitivity(Bool& CaseSensitive) const	{	}	//	default is whatever was passed in

	u32							GetLength(const TChar* pString);			//	get the length from some other type of string

	//	internal manipulation of buffer only!
	void						RemoveTerminator();							//	remove terminator from the string array
	void						SetTerminator();							//	make sure there is ONE terminator on the end of the string
	virtual void				OnStringChanged(u32 FirstChanged=0,s32 LastChanged=-1)	{	}	//	post-string change call

	void						AppendVaList(const TChar16* pFormat,va_list& v);	//	append formatted widestring - any %s's need to be widestring chars!
	void						AppendVaList(const TChar8* pFormat,va_list& v);		//	append formatted ascii string - %s is for ascii char*'s

protected:
	THeapArray<TChar>			m_DataArray;
};


//---------------------------------------------------------
//	append operator... specialise this for your own types. 
//	Annoyingly we can't make this a class/member operator because it would force us to include
//	TString.h in whatever type we have. Instead we have a global operator and then can redeclare the template
//	*then* overload it.
//	an alternative would be to standardise the string append for classes. eg. Any class we want to append
//	would implement GetString() and that would be called...
//---------------------------------------------------------
template<typename TYPE>
TString& operator<<(TString& String,const TYPE& Type)					
{	
	String.Append( Type );	
	return String;
}

//---------------------------
template<typename TYPE>
TString& operator<<(TString& String,const TYPE* Type)					
{	
	String.Append( Type );	
	return String;
}

template<typename TYPE>
void operator>>(const TString& String,TYPE& Type)
{
	TLDebug_Break(">> operator not implemented for this type");
}



//---------------------------------------------------------
//	same as TString but the array isn't dynamiccaly allocated, instead
//	it's a fixed array, much more effecient CPU wise, but more
//	expensvie memory wise - unless you make it small... the main drawback is
//	it's limited length
//---------------------------------------------------------
template<int SIZE>
class TBufferString : public TString
{
public:
	//	gr: note: do not use TString constructors as VTable isn't setup so doesn't use the TFixedArray
	//	gr: the fixed array is initialised with a size of 1 terminator so that the initial strlen() is okay.
	//		this is probably not really required as we don't (and shouldn't) use strlen() on the string anyway, but
	//		we do ensure this for printing purposes and such. We need to initialise the array because of alloc patterns (see TFixedArray)
	TBufferString()							: m_FixedDataArray(1,TLString_Terminator)	{	}
	TBufferString(const TString& String)	: m_FixedDataArray(1,TLString_Terminator)	{	Append( String );	}
	TBufferString(const TChar8* pString)	: m_FixedDataArray(1,TLString_Terminator)	{	Append( pString );	}
	TBufferString(const TChar16* pString)	: m_FixedDataArray(1,TLString_Terminator)	{	Append( pString );	}

	FORCEINLINE const TChar&		operator[](u32 Index) const					{	return GetCharAt(Index);	}

	//	gr: only need to overload = operator?
	FORCEINLINE TString&			operator=(const TChar8* pString)			{	Set( pString );	return *this;	}
	FORCEINLINE TString&			operator=(const TChar16* pString)			{	Set( pString );	return *this;	}
	FORCEINLINE TString&			operator=(const TString& String)			{	Set( String );	return *this;	}
	FORCEINLINE Bool				operator==(const TString& String) const		{	return IsEqual( String, TRUE );	}
	FORCEINLINE Bool				operator==(const TChar* pString) const		{	return IsEqual( pString, -1, TRUE );	}
	FORCEINLINE Bool				operator!=(const TString& String) const		{	return !IsEqual( String, TRUE );	}
	FORCEINLINE Bool				operator!=(const TChar* pString) const		{	return !IsEqual( pString, -1, TRUE );	}
	FORCEINLINE Bool				operator<(const TString& String) const		{	return IsLessThan( String );	}

	virtual TArray<TChar>&			GetStringArray()		{	return m_FixedDataArray;	}
	virtual const TArray<TChar>&	GetStringArray() const	{	return m_FixedDataArray;	}

protected:
	TFixedArray<TChar,SIZE>			m_FixedDataArray;
};


//	commonly used string for the stack(temp variables)
typedef TBufferString<512> TTempString;

//	string for debug-builds only.
//	todo: make a stub class for release builds where this class does nothing in case one is left in the code
//		or ifdef this out so it's not availible in release and so never used
typedef TBufferString<1024> TDebugString;






//---------------------------------------------------------
//	a normal string but forces all characters to be lower case
//	this speeds up string comparisons and makes some string operations
//	case insensitive.
//	very good for where case is not required, eg XML processing,
//	names of tags/properties etc
//---------------------------------------------------------
template<class BASESTRINGTYPE=TString>
class TStringLowercase : public BASESTRINGTYPE
{
public:
	//	gr: note: do not use TString constructors as VTable isn't setup, our post-append function won't be called
	//		so could be initialised with non-lowercase strings
	TStringLowercase()														{	}
	TStringLowercase(const TString& String)									{	BASESTRINGTYPE::Append( String );	}		
	TStringLowercase(const TChar8* pString)									{	BASESTRINGTYPE::Append( pString );	}
	TStringLowercase(const TChar16* pString)								{	BASESTRINGTYPE::Append( pString );	}

	virtual TChar				GetLowercaseCharAt(u32 Index) const				{	return BASESTRINGTYPE::GetCharAt( Index );	}

	FORCEINLINE const TChar&	operator[](u32 Index) const						{	return BASESTRINGTYPE::GetCharAt(Index);	}

	FORCEINLINE TString&		operator=(const TChar* pString)				{	BASESTRINGTYPE::Set( pString );	return *this;	}
	FORCEINLINE TString&		operator=(const TString& String)				{	BASESTRINGTYPE::Set( String );	return *this;	}
	FORCEINLINE Bool			operator==(const TString& String) const			{	return BASESTRINGTYPE::IsEqual( String, TRUE );	}
	FORCEINLINE Bool			operator==(const TChar* pString) const		{	return BASESTRINGTYPE::IsEqual( pString, -1, TRUE );	}
	FORCEINLINE Bool			operator!=(const TString& String) const			{	return !BASESTRINGTYPE::IsEqual( String, TRUE );	}
	FORCEINLINE Bool			operator!=(const TChar* pString) const		{	return !BASESTRINGTYPE::IsEqual( pString, -1, TRUE );	}
	FORCEINLINE Bool			operator<(const TString& String) const			{	return BASESTRINGTYPE::IsLessThan( String );	}

protected:
	virtual void				OnStringChanged(u32 FirstChanged=0,s32 LastChanged=-1);	//	post-string change call
	virtual void				ForceCaseSensitivity(Bool& CaseSensitive) const	{	CaseSensitive = FALSE;	}	//	lowercase string is always case-insesnsitive
};



//------------------------------------------------------
//	add string onto the end of the current string
//------------------------------------------------------
template<typename CHARTYPE>
TString& TString::Append(const CHARTYPE* pString,s32 Length)
{
	//	nothing to append...
	if ( Length == 0 || !pString )
		return (*this);

	//	string starts with a terminator
	const CHARTYPE& FirstChar = pString[0];
	if ( FirstChar == TLString_Terminator )
		return (*this);

	//	remove existing terminator to write straight after the current string
	RemoveTerminator();

	//	append string
	u32 Start = GetStringArray().GetSize();

	//	grow if we know the length
	if ( Length != -1 )
	{
		u32 NewLength = Start + Length;
		if ( !GetStringArray().SetSize( NewLength ) )
		{
			Length = GetLength() - Start;
			TTempString DebugString;
			DebugString.Appendf("Warning: String chopped - wanted to add %d - only appending %d chars (%d total)", NewLength-Start, Length, GetLength() );
			TLDebug_Print( DebugString );
		}

		TArray<TChar>& StringArray = GetStringArray();
		for ( u32 i=0;	i<(u32)Length;	i++ )
		{
			//	buffer allocated so write straight into buffer
			TChar Char = TLString::GetChar<TChar>( pString[i] );
			StringArray[Start+i] = Char;
		}
	}
	else
	{
		TArray<TChar>& StringArray = GetStringArray();
		const CHARTYPE* pChar = &pString[0];
		while ( (*pChar) != 0x0 )
		{
			//	buffer not allocated so do normal add routine
			TChar Char = TLString::GetChar<TChar>( *pChar );
			s32 NewIndex = StringArray.Add( Char );

			//	run out of memory/array
			if ( NewIndex == -1 )
				break;

			pChar++;
		}
	}

	//	ensure there's a terminator on the end
	SetTerminator();

	OnStringChanged( Start );

	return (*this);
}


//------------------------------------------------------
//	split string by SplitChar into array. if no cases of 
//	SplitChar then FALSE is return and no strings are added to the array
//------------------------------------------------------
template<class STRINGTYPE>
Bool TString::Split(const TChar& SplitChar,TArray<STRINGTYPE>& StringArray) const
{
	//	find first split point
	u32 SplitFrom = 0;
	s32 SplitTo = GetCharIndex( SplitChar );

	//	SplitChar not found
	if ( SplitTo == -1 )
		return FALSE;

	//	calc max number of strings we can split into
	u32 MaxSplitStrings = StringArray.GetMaxAllocSize();
	if ( MaxSplitStrings == 0 )
	{
		TLDebug_Break("Error; Array returned 0 for MaxAllocSize... should not happen and cannot continue with split");
		return false;
	}

	u32 LastCharIndex = GetCharGetLastIndex();

	while ( SplitFrom <= LastCharIndex )
	{
		if ( (s32)SplitFrom >= SplitTo )
		{
			//	gr: exception here, if the first character is a split character
			//		then push an empty string at the start.
			//		i've done this for the file system which wants to distingiush 
			//		.DS_Store into ""."DS_Store" (two strings so we know the 2nd is
			//		an extension. If that's not the case for general use (ie. don't 
			//		push an empty string first) then make the file system check for
			//		no filename before the extension
			if ( SplitFrom==0 && SplitTo==0 )
			{
				//	continue with code below to add an empty string
			}
			else
			{
				TDebugString Debug_String;
				Debug_String << "String split has got confused; SplitFrom: " << SplitFrom << ", SplitTo: " << SplitTo;				
				TLDebug_Break( Debug_String );
				break;
			}
		}

		//	make up new string
		STRINGTYPE SplitString;
		SplitString.Append( *this, SplitFrom, SplitTo-SplitFrom );

		//	add to array
		s32 AddIndex = StringArray.Add( SplitString );
		if ( AddIndex == -1 )
		{
			TLDebug_Break("gr: This shouldn't occur any more... end-of-array checked below!");
			//	cannot fit any more strings into this array
			return TRUE;
		}

		//	step over the character we split at, so +1
		SplitFrom = SplitTo + 1;

		//	if there is only one space left then put the whole of the rest of the string in
		s32 NextIndex = AddIndex + 1;

		//	MaxSplitStrings-1 == last-possible index in this array
		if ( NextIndex == MaxSplitStrings-1 )
		{
			SplitTo = -1;
		}
		else
		{
			//	find next split
			SplitTo = GetCharIndex( SplitChar, SplitFrom );
		}

		//	split at end of the string
		if ( SplitTo == -1 )
			SplitTo = LastCharIndex+1;
	}

	return TRUE;
}


//------------------------------------------------------
//	get all the strings seperated by the split chars (eg. white spaces)
//------------------------------------------------------
template<class STRINGTYPE>
Bool TString::Split(const TArray<TChar>& SplitChars,TArray<STRINGTYPE>& StringArray) const
{
	s32 StringStart = -1;

	//	walk through the string until we hit a split char
	u32 LastCharIndex = GetCharGetLastIndex();
	const TArray<TChar>& ThisString = GetStringArray();
	for ( u32 i=0;	i<=LastCharIndex+1;	i++ )
	{
		//	gr: we go up to past the last character so we keep all the end-of-string code together
		Bool IsTerminator = (i == LastCharIndex+1);
		Bool IsSplitChar = IsTerminator;

		//	check for a non-string character
		if ( !IsSplitChar )
		{
			const TChar& Char = ThisString[i];
			IsSplitChar = SplitChars.Exists( Char );
		}

		//	end of current string
		if ( IsSplitChar )
		{
			//	more white space, not doing a string atm...
			if ( StringStart == -1 )
				continue;

			//	terminate current string

			//	get a new string in the array
			STRINGTYPE* pNewString = StringArray.AddNew();
			if ( !pNewString )
			{
				//	failed to alloc. run out of space, or memory etc. not sure what to return
				TLDebug_Break("Failed to allocate another string for the split... gr: not sure what to return... success or fail?");
				return !StringArray.IsEmpty();
			}

			//	work out the length of the string we've found
			s32 Length = (i - StringStart);
			if ( Length <= 0 )
			{
				TLDebug_Break("coding error: string in string is too short");
				Length = 1;
			}

			//	put our part of a string into this new string
			pNewString->Append( *this, (u32)StringStart, Length );

			//	reset start for next string
			StringStart = -1;
			continue;
		}

		//	real character! store the index if it's the start of a new string
		if ( StringStart == -1 )
			StringStart = i;

		//	wait for something to terminate the current string
		continue;
	}

	return !StringArray.IsEmpty();
}


//------------------------------------------------------
//	extract N ints (no more, no less) into a TypeN<int> type - gr: this should support u8,u16 etc, but might throw up type-cast warnings. We could build in the signed/range checking into this routine...
//------------------------------------------------------
template<class TYPE_N>
Bool TString::GetInteger(TYPE_N/*<int>*/& IntN) const
{
	//	gr: replace this with an in-place array when they're implemented. will save copying the array afterwards
	//	TInPlaceArray<float> ExtractedFloats( FloatN.GetData(), FloatN.GetSize() );
	TFixedArray<int,4> ExtractedIntegers;
	if ( !GetIntegers( ExtractedIntegers) )
		return false;

	//	if number of integers mis-matches what we want then fail
	if ( ExtractedIntegers.GetSize() != IntN.GetSize() )
		return false;

	//	put ints we extracted into the variable
	TLMemory::CopyData( IntN.GetData(), ExtractedIntegers.GetData(), IntN.GetSize() );
	return true;
}


//------------------------------------------------------
//	extract N floats (no more, no less) into a TypeN<float> type
//------------------------------------------------------
template<class TYPE_N>
Bool TString::GetFloat(TYPE_N/*<float>*/& FloatN) const
{
	//	gr: replace this with an in-place array when they're implemented. will save copying the array afterwards
	//	TInPlaceArray<float> ExtractedFloats( FloatN.GetData(), FloatN.GetSize() );
	TFixedArray<float,4> ExtractedFloats;
	if ( !GetFloats( ExtractedFloats ) )
		return false;

	//	if number of floats mis-matches what we want then fail
	if ( ExtractedFloats.GetSize() != FloatN.GetSize() )
		return false;

	//	put floats we extracted into the variable
	TLMemory::CopyData( FloatN.GetData(), ExtractedFloats.GetData(), FloatN.GetSize() );
	return true;
}


//------------------------------------------------------
//	post-string change call
//------------------------------------------------------
template<class BASESTRINGTYPE>
void TStringLowercase<BASESTRINGTYPE>::OnStringChanged(u32 FirstChanged,s32 LastChanged)
{
	if ( LastChanged < 0 )
		LastChanged = BASESTRINGTYPE::GetLength();

	TArray<TChar>& StringArray = BASESTRINGTYPE::GetStringArray();
	for ( u32 i=FirstChanged;	i<(u32)LastChanged;	i++ )
	{
		TLString::SetCharLowercase( StringArray[i] );
	}
}



//--------------------------------------------------------
//	append integer
//	I'd add this as a member function, but makes a good example for overloaded appends for different types
//--------------------------------------------------------
template<>
FORCEINLINE TString& operator<<(TString& String,const u32& Value)		
{
	TFixedArray<TChar,12> IntegerChars;
	u32 i=Value;

	while ( true )
	{
		int OfTen = i % 10;
		IntegerChars.InsertAt( 0, '0' + OfTen );

		//	won't shrink any further
		if ( i < 10 )
			break;

		//	next 10th
		i /= 10;
	}

	//	append our array of characters we've made up
	String.Append( IntegerChars.GetData(), IntegerChars.GetSize() );

	return String;
}
template<> FORCEINLINE TString& operator<<(TString& String,const Type2<u32>& v)	{	return TLString::AppendType2(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type3<u32>& v)	{	return TLString::AppendType3(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type4<u32>& v)	{	return TLString::AppendType4(String,v);	}


//--------------------------------------------------------
//	append signed integer
//--------------------------------------------------------
template<>
FORCEINLINE TString& operator<<(TString& String,const s32& Value)		
{
	//	if negative, append a - then treat as if it was unsigned
	if ( Value < 0 )
		String.Append('-');

	u32 v = Value < 0 ? -Value : Value;
	String << v;
	return String;
}
template<> FORCEINLINE TString& operator<<(TString& String,const Type2<s32>& v)	{	return TLString::AppendType2(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type3<s32>& v)	{	return TLString::AppendType3(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type4<s32>& v)	{	return TLString::AppendType4(String,v);	}



template<> FORCEINLINE TString& operator<<(TString& String,const u8& Value)		{	u32 v = Value;	String << v;	return String;	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type2<u8>& v)	{	return TLString::AppendType2(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type3<u8>& v)	{	return TLString::AppendType3(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type4<u8>& v)	{	return TLString::AppendType4(String,v);	}

template<> FORCEINLINE TString& operator<<(TString& String,const u16& Value)	{	u32 v = Value;	String << v;	return String;	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type2<u16>& v)	{	return TLString::AppendType2(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type3<u16>& v)	{	return TLString::AppendType3(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type4<u16>& v)	{	return TLString::AppendType4(String,v);	}

template<> FORCEINLINE TString& operator<<(TString& String,const s8& Value)		{	s32 v = Value;	String << v;	return String;	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type2<s8>& v)	{	return TLString::AppendType2(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type3<s8>& v)	{	return TLString::AppendType3(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type4<s8>& v)	{	return TLString::AppendType4(String,v);	}

template<> FORCEINLINE TString& operator<<(TString& String,const s16& Value)	{	s32 v = Value;	String << v;	return String;	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type2<s16>& v)	{	return TLString::AppendType2(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type3<s16>& v)	{	return TLString::AppendType3(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type4<s16>& v)	{	return TLString::AppendType4(String,v);	}

template<> FORCEINLINE TString& operator<<(TString& String,const float& Value)	{	String.Appendf("%2.2f", Value );	return String;	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type2<float>& v)	{	return TLString::AppendType2(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type3<float>& v)	{	return TLString::AppendType3(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type4<float>& v)	{	return TLString::AppendType4(String,v);	}

template<> FORCEINLINE TString& operator<<(TString& String,const bool& Value)		{	String << (Value ? "True" : "False");	return String;	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type2<bool>& v)	{	return TLString::AppendType2(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type3<bool>& v)	{	return TLString::AppendType3(String,v);	}
template<> FORCEINLINE TString& operator<<(TString& String,const Type4<bool>& v)	{	return TLString::AppendType4(String,v);	}


