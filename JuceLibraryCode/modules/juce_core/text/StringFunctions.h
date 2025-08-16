#pragma once
#pragma warning (disable : 4244)

namespace StringFunctions
{

typedef juce::String jString;
typedef juce::String::CharPointerType jCharPointer;


//simliar to juce::StringArray::joinIntoString
inline juce::String mergeArray(const juce::StringArray& mergeArray, char seperator, bool skipEmptyStrings = true)
{
	juce::String output;
	for (int i = 0; i < mergeArray.size(); i++)
	{
		if (i + 1 == mergeArray.size()) //last element
			output += mergeArray[i];
		else if (!(skipEmptyStrings && mergeArray[i].isEmpty()))
			output += mergeArray[i] + seperator;
	}
	return output;
}
inline juce::String mergeArray(const juce::StringArray& mergeArray, juce::String seperator, bool skipEmptyStrings = true)
{
	juce::String output;
	for (int i = 0; i < mergeArray.size(); i++)
	{
		if (i + 1 == mergeArray.size()) //last element
			output += mergeArray[i];
		else if (!(skipEmptyStrings && mergeArray[i].isEmpty()))
			output += mergeArray[i] + seperator;
	}
	return output;
}

inline bool anyArrayElementContains(const juce::StringArray& array, const juce::String& search, bool ignoreCase = false)
{
	if (ignoreCase)
	{
		for (auto& i : array)
			if (i.containsIgnoreCase(search))
				return true;
	}
	else
	{
		for (auto& i : array)
			if (i.contains(search))
				return true;
	}
	return false;
}
inline bool stringContainsAnyArrayElement(const juce::StringArray& array, const juce::String& stringToSearch, bool ignoreCase = false)
{
	if (ignoreCase)
	{
		for (auto& i : array)
			if (stringToSearch.containsIgnoreCase(i))
				return true;
	}
	else
	{
		for (auto& i : array)
			if (stringToSearch.contains(i))
				return true;
	}
	return false;
}

inline juce::String getLine(const juce::String& multiLineString, int lineIndex)
{
	juce::String output;
	juce::String::CharPointerType reader = multiLineString.getCharPointer();

	for (int i = 0; i < lineIndex; i++)
	{
		while (*reader != '\n')
		{
			if (reader.isEmpty())
				return output;
			reader++;
		}
		reader++; //after new line char
	}

	while (*reader != '\n')
	{
		if (reader.isEmpty())
			return output;

		output += *reader; //get line
		reader++;
	}
	return output;
}
inline int getLinePosition(const juce::String& multiLineString, int lineIndex)
{
	juce::String output;
	juce::String::CharPointerType reader = multiLineString.getCharPointer();

	for (int i = 0; i < lineIndex; i++)
	{
		while (*reader != '\n')
		{
			if (reader.isEmpty())
				return -1;
			reader++;
		}
		reader++; //after new line char
	}
	return reader.getAddress() - multiLineString.begin().getAddress();
}
inline jString getLineAtPosition(const juce::String& multiLineString, int position)
{
	juce::String output;
	juce::String::CharPointerType reader = multiLineString.getCharPointer();
	
	jCharPointer currentLineStart = reader;
	while (true)
	{
		if (*reader == '\n')
		{
			reader++;
			if (reader.isNotEmpty())
				currentLineStart = reader;
		}
	}
	//{
	//	while (*reader != '\n')
	//	{
	//		if (reader.isEmpty())
	//			return -1;
	//		reader++;
	//	}
	//	reader++; //after new line char
	//}
	return "";
}

inline int getLineCount(const juce::String& multiLineString, bool countTrailingLine = true)
{
	int lineCount = 1;
	juce::String::CharPointerType reader = multiLineString.getCharPointer();
	while (reader.isNotEmpty())
	{
		if (*reader == '\n')
			lineCount++;
		reader++;
	}
	if (!countTrailingLine && *reader.operator--() == '\n')
		lineCount--;

	return lineCount;
}

//will stop when the condition is true or the reader is empty
//passing an empty condition will just read the rest of the string
inline juce::String readUntilCondition(juce::String::CharPointerType& reader, std::function<bool(juce::String::CharPointerType& reader)> condition)
{
	juce::String output;
	while (reader.isNotEmpty())
	{
		if (condition && condition(reader))
			break;
		output += *reader;
		++reader;
	}
	return output;
}

inline juce::StringArray split(const juce::String& stringToBreakUp, const juce::String& separator)
{
	juce::StringArray output;
	int reader = 0;
	int separatorIndex = stringToBreakUp.indexOf(reader, separator);
	//auto reader = stringToBreakUp.getCharPointer();
	while (separatorIndex != -1)
	{
		juce::String newString = stringToBreakUp.substring(reader, separatorIndex);
		if (newString.isNotEmpty())
			output.add(std::move(newString));
		reader = separatorIndex + separator.length();

		separatorIndex = stringToBreakUp.indexOf(reader, separator);
	}
	if (reader != stringToBreakUp.length())
		output.add(stringToBreakUp.substring(reader));

	return output;
}

inline juce::String boolToString(bool value)
{
	if (value)
		return "true";
	else
		return "false";
}

} //namespace StringFunctions