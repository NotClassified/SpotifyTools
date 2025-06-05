#pragma once

#include <JuceHeader.h>
#include "Globals.h"

#define CALCULATE_GRID_POSITIONS 0

namespace json
{

class Stream;
class Property;
class Array;

enum class Type : uint8_t { None, Object, Array, String, Number, Bool, Null, Property };

enum class ScopeFormat { Empty, Collapsed, Expanded };

class Position
{
public:
	//==============================================================================
	Position() {}
	Position(Stream* jsonStream, int readPosition = 0);
	Position(Position& copy);
	Position(Position&& move) noexcept;
	virtual ~Position();

	bool isType(Type compareType) const { return type == compareType; }

	bool operator!=(Position compare) {}

	explicit operator bool() const;
	bool isValid() const { return type != Type::None; }
	bool isNotValid() const { return type == Type::None; }
	bool isNull() { return type == Type::Null; }

	//==============================================================================
	Position operator[](const juce::String& propertyKey);
	Position getProperty(const juce::String& propertyKey);
	juce::StringArray getPropertyKeys();

	juce::String getString();
	juce::String getString(bool applyEscapeSequences, bool ignoreHtmlText = false);
	bool tryGetString(juce::String& out);

	juce::String getInt();
	bool tryGetInt(juce::String& out);
	int getIntValue();
	bool tryGetIntValue(int& out);
	bool tryGetIntValue(char& out);

	juce::String getNumber();
	bool tryGetNumber(juce::String& out);
	float getFloatValue();
	bool tryGetFloatValue(float& out);

	bool getBool();
	bool tryGetBool(bool& out);

	Array getArray();

	void setString(juce::String newString);
	void setInt(juce::String newInt);

	virtual void jsonResized(int centerPosition, int shiftAmount);

	//==============================================================================
protected:
	Stream* p_stream = nullptr;
	int p_resizeListenerIndex = -1;

public:
	int readPosition = 0;
	Type type = Type::None;

	//==============================================================================
};
class Property : public Position
{
public:
	Property();
	Property(const Property& copy);
	Property(Stream* jsonStream, int valueReadPosition, int keyReadPosition = 0, const juce::String& key = "");
	Property(Position& copy, const juce::String& key = "");
	Property(Position&& move, const juce::String& key = "");

	bool operator<(const Property& compare) { return key < compare.key; }

	juce::String key;
	//property's value position is "readPosition"
	int keyReadPosition = 0;
};
class Array : public Position
{
public:
	//==============================================================================
	Array() {}
	Array(Stream* jsonStream, int position = 0);
	Array(Position& jsonProperty);

	//==============================================================================
	//overridden methods:

	explicit operator bool() const;

	Position operator[](const juce::String& propertyKey) = delete;
	Position getProperty(const juce::String& propertyKey) = delete;
	juce::StringArray getPropertyKeys() = delete;
	juce::String getString() = delete;
	bool tryGetString(juce::String& out) = delete;
	juce::String getInt() = delete;
	bool tryGetInt(juce::String& out) = delete;
	int getIntValue() = delete;
	bool tryGetIntValue(int& out) = delete;
	bool tryGetIntValue(char& out) = delete;
	juce::String getNumber() = delete;
	bool tryGetNumber(juce::String& out) = delete;
	float getFloatValue() = delete;
	bool tryGetFloatValue(float& out) = delete;
	bool getBool() = delete;
	bool tryGetBool(bool& out) = delete;
	void setString(juce::String newString) = delete;
	void setInt(juce::String newInt) = delete;

	Array getArray();

	void jsonResized(int centerPosition, int shiftAmount) override;

	//==============================================================================
	void goToStart();

	Position& operator[](int index);
	bool operator++();
	bool next();

	struct Iterator
	{
		Iterator(Array& owningArray) : owningArray(owningArray)
		{
		}

		bool operator!=(Iterator&) { return owningArray.currentElement.isValid(); };
		void operator++() { ++owningArray; }
		Position& operator*() { return owningArray.currentElement; }
		Iterator& operator+(int increment) { owningArray[owningArray.currentIndex + increment]; return *this; }

		bool operator<(int compareIndex) { return owningArray.currentIndex < compareIndex; }
		bool operator<=(int compareIndex) { return owningArray.currentIndex <= compareIndex; }
		bool operator>(int compareIndex) { return owningArray.currentIndex > compareIndex; }
		bool operator>=(int compareIndex) { return owningArray.currentIndex >= compareIndex; }

		Array& owningArray;
	};
	Iterator begin()
	{
		goToStart();
		return Iterator(*this);
	}
	Iterator end() { return Iterator(*this); }

	int getSize();

	juce::StringArray getStrings();
	bool tryGetStrings(juce::StringArray& out);

	juce::StringArray getInts();
	bool tryGetInts(juce::StringArray& out);

	int getArrayElementEndPosition(bool afterEndChar);

	//==============================================================================
	int arrayStartPosition = 0;
	int currentIndex = 0;
	Position currentElement;

	//==============================================================================
};

class Stream
{
public:
	//==============================================================================
	Stream();
	Stream(const juce::File& jsonFile, bool shouldBeReadOnly = false);
	Stream(juce::String jsonText, bool shouldBeReadOnly = false);

	void newStream(const juce::File& jsonFile);

public:
	//==============================================================================
	//start position methods:

	Position& getStart() { return m_start; }
	explicit operator bool() const { return m_start.isValid(); }

	Position operator[](const juce::String& propertyKey) { return findProperty(m_start, propertyKey); }
	Position findProperty(const juce::String& propertyKey) { return findProperty(m_start, propertyKey); }
	Property getProperty(const juce::String& propertyKey) { return getProperty(m_start, propertyKey); }
	juce::StringArray getPropertyKeys() { return getPropertyKeys(m_start); }

	Array getArray() { return getArray(m_start); }

	//==============================================================================
	//reading json data

	struct StringReadOptions
	{
	public:
		StringReadOptions(bool applyEsacpeSequences = false,
						  bool skipHtmlSequences = false,
						  bool handleNonUTF8 = false);

		struct convertUTF8Pair
		{
			convertUTF8Pair() {}
			convertUTF8Pair(const juce::juce_wchar& convertFrom, char convertTo) : convertFrom(convertFrom), convertTo(convertTo) {}

			bool operator<(convertUTF8Pair& compare) { return convertFrom < compare.convertFrom; }
			juce::juce_wchar convertFrom = 0;
			char convertTo = 0;
		};

		void addUTF8ConversionPair(juce::juce_wchar convertFrom, char convertTo);
		char convertToUTF8(juce::juce_wchar convertCharacter) const;

		bool applyEsacpeSequences;
		bool skipHtmlSequences;

		bool handleNonUTF8;
	private:
		juce::Array<convertUTF8Pair> m_utf8ConversionSet;
	};

	const juce::String& getEntireJsonText() { return m_jsonText; }

	Position findProperty(Position& jsonObject, const juce::String& key);
	Property getProperty(Position& jsonObject, const juce::String& key);
	juce::StringArray getPropertyKeys(Position& jsonObject);
	juce::Array<Property> getObjectProperties();
	juce::Array<Property> getObjectProperties(Position& jsonObject);
	int getPropertyKeyPosition(Property& jsonProperty);

	Array getArray(Position& jsonProperty);
	void setNextArrayElement(Array& fromArray);
	//end char examples: '}', ']', '\"', '1', etc.
	int getArrayElementEndPosition(Array& jsonArray, bool afterEndChar);

	juce::String getString(Position& jsonString);
	juce::String getString(Position& jsonString, bool applyEscapeSequences, bool ignoreHtmlText);
	//starts at '\"', end char is the string's end quote '\"'
	juce::String getString();
	//starts at '\"', end char is the string's end quote '\"'
	juce::String getString(const StringReadOptions& readOptions);

	juce::String getInt(Position& jsonInt);
	juce::String getNumber(Position& jsonNumber);
	//starts at first digit (or negative sign), ends after last digit
	juce::String getNumber();
	bool getBool(Position& jsonBool);
	//does not move reader
	bool getBool();

	//starts at "//" (single-line) or "/*" (multi-line), ends after new line (single-line) or after "*/" (multi-line)
	juce::String getComment();

	//starts at '{' or '['
	//doesn't handle the case of scope being empty and multi-lined
	ScopeFormat getScopeFormat(bool returnToStartPosition);

public:
	//==============================================================================
	//writing

	void setData(int startPosition, int endPosition, const juce::String& newData, int setReaderPosition = 0);

	void setString(Position& jsonProperty, const juce::String& newString);
	void setInt(Position& jsonProperty, const juce::String& newInt);

	void flushJson();

	//return new index
	int addResizeListener(Position* listener);
	void swapResizeListener(int listenerIndex, Position* listener);
	void removeResizeListener(int listenerIndex);
	void removeResizeListener(Position* listener);

private:
	//@param centerPosition - position of the resizing
	//@param shiftAmount - amount of bytes shifted
	void jsonResized(int centerPosition, int shiftAmount);

public:
	//==============================================================================
	//reading json text

	//copies the reader, know that writing to this json will make this reader invalid
	juce::String::CharPointerType getReader() { return m_reader; }
	int getReadPosition();
	char getCurrentChar();
	bool isAtChar(char compare) { return m_currentChar == compare; }
	bool isEndOfJson();

	void goToPosition(Position& jsonType);
	void goToPosition(int newPosition);
	//start of the entire text
	void goToJsonTextStart();
	//start object or array
	void goToJsonStart() { goToPosition(m_start); }

	void setJsonGridPositions(Position& jsonType);

	//before null termination character
	void readLastPosition();

	void readNextPosition();
	void readNextPosition(int increment);
	void readPreviousPosition();
	void readPreviousPosition(int increment);

	bool trySkipWhitespaces();
	//includes space, carriage return, new line, tab
	bool isAtWhitespace();

	bool isAtSingleLineComent();
	bool isAtMultiLineComent();

	bool isAtScopeStart() { return m_currentChar == '{' || m_currentChar == '['; }
	bool isAtScopeEnd() { return m_currentChar == '}' || m_currentChar == ']'; }

	int readNextCharPosition(char findChar);
	bool tryReadNextCharPosition(char findChar);
	void readPositionAfterChar(char findChar);
	bool tryReadPositionAfterChar(char findChar);
	int readPreviousCharPosition(char findChar);

	//==============================================================================
	//skipping

	//includes single-line comment, multi-line comment, space, carriage return, new line, tab
	void skipCommentsAndWhitespaces();
	//includes space, carriage return, new line, tab
	void skipWhitespacesAndCommas();

	//starts at "//" (single-line) or "/*" (multi-line), ends after new line (single-line) or after "*/" (multi-line)
	//@return comment skipped
	bool skipComment();
	//includes space, carriage return, new line, tab
	void skipWhitespaces();

	//starts at '{' or '[', end char is '}' or ']'
	void skipScope(bool afterEndChar);
	//starts at '{', end char is '}'
	void skipObject(bool afterEndChar);
	//starts at '[', end char is ']'
	void skipArray(bool afterEndChar);
	//starts at '\"', ends at ':' or the property's value if specified
	void skipPropertyKey(bool goToPropertyValue);

	//starts at '\"', end char is the string's end quote '\"'
	void skipString(bool afterEndChar);

	//starts at first digit (or negative sign), end char is the last digit
	void skipNumber(bool afterEndChar);
	//starts at first digit (or negative sign), end char is the last digit
	void skipInt(bool afterEndChar);
	//starts at first digit (or negative sign), end char is the last digit
	void skipFloat(bool afterEndChar);

	//starts at first letter, end char is the last letter
	void skipBool(bool afterEndChar);
	//starts at first letter, end char is the last letter
	void skipNull(bool afterEndChar);

	//==============================================================================
public:
	bool readOnly = false;

private:
	juce::File m_jsonFile;
	juce::String m_jsonText;
	juce::Array<Position*> m_resizeListeners{};
	Position m_start;

	juce::String::CharPointerType m_reader;
	char m_currentChar = 0;
	int m_readPosition = 0;
	int line = 0;
	int lineChar = 0;

	//==============================================================================
private:
	JUCE_LEAK_DETECTOR(Stream)
};

//==============================================================================

Type getTypeFromChar(char startChar);
Type getTypeFromChar(juce::juce_wchar startChar);
inline bool isScopeType(Type checkType) { return checkType == Type::Object || checkType == Type::Array; }

//==============================================================================

class Tree
{
public:
	//==============================================================================
	Tree(Stream* stream) : m_stream(stream)
	{
		initialize();
	}

	~Tree()
	{
		delete m_base;
	}

	//==============================================================================
	enum class ExtraType : uint8_t { None, SingleLineComment, MultiLineComment, EmptyLine};

	struct SingleLineComment;
	struct MultiLineComment;
	struct EmptyLine;

	struct Extra
	{
		Extra(ExtraType type = ExtraType::None) : type(type) {}
		virtual ~Extra() {};

		SingleLineComment* toSingleLineComment() const { jassert(ExtraType::SingleLineComment == type); return (SingleLineComment*)this; }
		MultiLineComment* toMultiLineComment() const { jassert(ExtraType::MultiLineComment == type); return (MultiLineComment*)this; }
		EmptyLine* toEmptyLine() const { jassert(ExtraType::EmptyLine == type); return (EmptyLine*)this; }

		ExtraType type;
	};
	struct SingleLineComment : Extra
	{
		SingleLineComment(const juce::String& data = "", bool onNewLine = true) : Extra(ExtraType::SingleLineComment), data(data), onNewLine(onNewLine) {}
		~SingleLineComment() override {}
		juce::String data;
		bool onNewLine; //deprecated
	};
	struct MultiLineComment : Extra
	{
		MultiLineComment(const juce::String& data = "", bool onNewLine = true) : Extra(ExtraType::MultiLineComment), data(data), onNewLine(onNewLine) {}
		~MultiLineComment() override {}
		juce::String data;
		bool onNewLine; //deprecated
	};
	struct EmptyLine : Extra
	{
		EmptyLine(int count = 0) : Extra(ExtraType::EmptyLine), count(count) {}
		~EmptyLine() override {}
		int count;
	};

	//==============================================================================
	struct JsonString;
	struct JsonNumber;
	struct JsonBool;
	struct Property;
	struct Object;
	struct Array;

	struct Entity
	{
		Entity(Type type = Type::None) : type(type) {}
		virtual ~Entity() 
		{
			for (auto& e : extras)
				delete e;
		}

		JsonString& toJsonString() const { jassert(getEntityTypeEnum<JsonString>() == type); return *(JsonString*)this; }
		JsonNumber& toJsonNumber() const { jassert(getEntityTypeEnum<JsonNumber>() == type); return *(JsonNumber*)this; }
		JsonBool& toJsonBool() const { jassert(getEntityTypeEnum<JsonBool>() == type); return *(JsonBool*)this; }
		Property& toProperty() const { jassert(getEntityTypeEnum<Property>() == type); return *(Property*)this; }
		Object& toObject() const { jassert(getEntityTypeEnum<Object>() == type); return *(Object*)this; }
		Array& toArray() const { jassert(getEntityTypeEnum<Array>() == type); return *(Array*)this; }

		//comments before/above entity
		Entity* parent = nullptr;
		std::vector<Extra*> extras;
		Type type;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Entity)
	};
	struct JsonString : Entity
	{
		JsonString(const juce::String& data = "") : Entity(Type::String), data(data) {}
		~JsonString() override {}
		operator juce::String&() { return data; }
		juce::String data;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JsonString)
	};
	struct JsonNumber : Entity
	{
		JsonNumber(const juce::String& data = "") : Entity(Type::Number), data(data) {}
		~JsonNumber() override {}
		juce::String data;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JsonNumber)
	};
	struct JsonBool : Entity
	{
		JsonBool(bool data = false) : Entity(Type::Bool), data(data) {}
		~JsonBool() override {}
		operator bool() { return data; }
		bool data;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JsonBool)
	};
	struct Property : Entity
	{
		Property(const juce::String& key = "", Entity* value = nullptr) : Entity(Type::Property), key(key), value(value) {}
		~Property() override
		{
			delete value;
		}
		juce::String key;
		Entity* value;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Property)
	};
	struct Object : Entity
	{
		Object() : Entity(Type::Object) {}
		~Object() override
		{
			for (auto& p : properties)
				delete p;
			for (auto& e : endExtras)
				delete e;
		}
		Property* operator[](const juce::String& key) { return getProperty(key); }
		Property* getProperty(const juce::String& key)
		{
			for (auto& p : properties)
			{
				if (p->key == key)
					return p;
			}
			return nullptr;
		}

		std::vector<Property*> properties;
		//extras after the last property
		std::vector<Extra*> endExtras;
		ScopeFormat scopeFormat = ScopeFormat::Empty;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Object)
	};
	struct Array : Entity
	{
		Array() : Entity(Type::Array) {}
		~Array() override
		{
			for (auto& e : elements)
				delete e;
			for (auto& e : endExtras)
				delete e;
		}
		std::vector<Entity*> elements;
		//extras after the last element
		std::vector<Extra*> endExtras;
		ScopeFormat scopeFormat = ScopeFormat::Empty;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Array)
	};

	//==============================================================================
	static bool isEqualJsonData(Stream& stream1, Stream& stream2, bool ignoreCommas = true);

	//==============================================================================
	void initialize();

private:
	//use initializeExtras() for the extras argument
	void initializeJson(Entity*& currentEntity, std::vector<Extra*>& extras);
	void initializeJsonProperty(Property*& currentProperty, std::vector<Extra*>& extras);
	//skips whitespaces afterwards
	std::vector<Extra*> initializeExtras(bool newLine);

public:
	bool isValid() { return m_stream && m_stream->getStart().isValid(); }

	void debugFormat()
	{
		if (!isValid())
			return;
		juce::File(g_debugPath + "\\DebugJsonFormat.json").replaceWithText(getText());
	}
	juce::String getText()
	{
		if (!isValid())
			return juce::String();
		juce::String output;
		output.preallocateBytes(m_stream->getEntireJsonText().length() * 2);
		createFormat(m_base, output, 0);
		return output;
	}

	//==============================================================================
	//reading tree structure:

	Entity& getBase() { return *m_base; }

	vArray<Tree::Property*> findStringProperties(const juce::String& key, const juce::String& value);
	void findStringProperties(const juce::String& key, const juce::String& value, 
														  Entity& currentEntity, vArray<Tree::Property*>& output);

private:
	//==============================================================================
	template<class fromEntityType>
	static Type getEntityTypeEnum()
	{
		if (typeid(fromEntityType) == typeid(Object))
			return Type::Object;
		if (typeid(fromEntityType) == typeid(Property))
			return Type::Property;
		if (typeid(fromEntityType) == typeid(Array))
			return Type::Array;
		if (typeid(fromEntityType) == typeid(JsonString))
			return Type::String;
		if (typeid(fromEntityType) == typeid(JsonNumber))
			return Type::Number;
		if (typeid(fromEntityType) == typeid(JsonBool))
			return Type::Bool;
		jassertfalse; //could not find entity type
		return Type::None;
	}

	//==============================================================================
	void createFormat(const Entity* currentEntity, juce::String& output, int currentIndent, bool indentBeforeEntity = false);
	void createExtras(const std::vector<Extra*>& extras, juce::String& output, int indentCount);
	void createObject(const Object& currentObject, juce::String& output, int currentIndent, ScopeFormat scopeFormat);
	void createArray(const Array& currentArray, juce::String& output, int currentIndent, ScopeFormat scopeFormat);

	bool containsComments(const Entity* entity);
	bool containsComments(const std::vector<Extra*>& extras);

	//ignores the extas above objectEntity
	bool containsExtras(const Object* objectEntity);
	//ignores the extas above arrayEntity
	bool containsExtras(const Array* arrayEntity);
	bool containsExtras(const Entity* entity);

	bool containsNestedScopes(const Object& checkScope);
	bool containsNestedScopes(const Array& checkScope);
	bool checkIfAnyPropertyMeetsCondition(Object& checkObject, std::function<bool(Property*)> condition);
	bool containsNestedObjects(const Array& checkScope);

	juce::String getIndentString(int indentCount);

	//==============================================================================
	//for custom formatting

	//recursively searches all parents
	bool isOwnedByProperty(const Entity* child, juce::String propertyKey);

private:
	//==============================================================================

	Entity* m_base = nullptr;
	Stream* m_stream;

	const char* lineFeed = "\r\n";

	//==============================================================================
};

} //namespace json