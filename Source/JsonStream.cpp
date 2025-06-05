#include "JsonStream.h"

namespace json
{
Stream::Stream() :
    m_jsonFile(),
    m_jsonText(""),
    m_reader(m_jsonText.getCharPointer()),
    m_currentChar(0),
    m_start()
{
}
Stream::Stream(const juce::File& jsonFile, bool shouldBeReadOnly) :
    m_jsonFile(jsonFile),
    m_jsonText(jsonFile.loadFileAsString()),
    m_reader(m_jsonText.getCharPointer()),
    m_currentChar(*m_reader.getAddress()),
    readOnly(shouldBeReadOnly),
    m_start(Position(this, 0))
{
    if (m_reader.isEmpty())
        return;

    skipCommentsAndWhitespaces();
    if (m_currentChar == '{')
        m_start.type = Type::Object;
    else if (m_currentChar == '[')
        m_start.type = Type::Array;

    m_start.readPosition = m_readPosition;
}
Stream::Stream(juce::String jsonText, bool shouldBeReadOnly) :
    m_jsonText(jsonText),
    m_reader(m_jsonText.getCharPointer()),
    m_currentChar(*m_reader.getAddress()),
    readOnly(shouldBeReadOnly),
    m_start(Position(this, 0))
{
    if (jsonText.isEmpty())
        return;

    skipCommentsAndWhitespaces();
    if (m_currentChar == '{')
        m_start.type = Type::Object;
    else if (m_currentChar == '[')
        m_start.type = Type::Array;

    m_start.readPosition = m_readPosition;
}

void Stream::newStream(const juce::File& jsonFile)
{
    m_jsonFile = jsonFile;
    m_jsonText = jsonFile.loadFileAsString();
    goToJsonTextStart();
    if (m_reader.isEmpty())
        return;

    skipCommentsAndWhitespaces();
    if (m_currentChar == '{')
        m_start.type = Type::Object;
    else if (m_currentChar == '[')
        m_start.type = Type::Array;

    m_start.readPosition = m_readPosition;

    m_resizeListeners.clear();
}

Position Stream::findProperty(Position& jsonObject, const juce::String& key)
{
    if (!jsonObject.isType(Type::Object))
        return Position();

    goToPosition(jsonObject);
    readNextPosition(); //after '{'
    skipCommentsAndWhitespaces();

    auto keyReader = key.getCharPointer();
    int currentScope = 0;
    while (true)
    {
        if (m_currentChar == '\"') //found a key
        {
            readNextPosition();
            keyReader = key.getCharPointer();
            while (m_currentChar == *keyReader.getAddress()) //match keys
            {
                readNextPosition();
                keyReader++;

                if (keyReader.isEmpty()) //end of matching key
                {
                    if (m_currentChar == '\"') //end of current key?
                    {
                        readPositionAfterChar(':');
                        skipCommentsAndWhitespaces();

                        Position output(this);
                        output.type = getTypeFromChar(m_currentChar);
                        output.readPosition = m_readPosition; //start position of property's value ('{', '\"', '1', etc.)
                        setJsonGridPositions(output);
                        return output;
                    }
                    else
                        break;
                }
            }

            //keys do not match, go to the end of this property
            readNextCharPosition('\"');
            readPositionAfterChar(':');
            skipCommentsAndWhitespaces();
            switch (getTypeFromChar(m_currentChar))
            {
                case Type::Object: skipObject(true); break;
                case Type::Array: skipArray(true); break;
                case Type::String: skipString(true); break;
                case Type::Number: skipNumber(true); break;
                case Type::Bool: skipBool(true); break;
                case Type::Null: skipNull(true);  break;
                default: jassertfalse; break;
            }
        }
        else if (m_currentChar == '{')
        {
            currentScope++;
        }
        else if (m_currentChar == '}')
        {
            currentScope--;
            if (currentScope < 0)
            {
                return Position(); //key does not exist in current object
            }
        }
        else
            DBG("Stream::getProperty() got an unexpected char '" << m_currentChar << "' (" << ((int)m_currentChar) << ")");

        skipCommentsAndWhitespaces();
        if (m_currentChar == ',')
        {
            readNextPosition();
            skipCommentsAndWhitespaces();
        }
    }

    return Position();
}

Property Stream::getProperty(Position& jsonObject, const juce::String& key)
{
    return Property(findProperty(jsonObject, key), key);
}

juce::StringArray Stream::getPropertyKeys(Position& jsonObject)
{
    if (!jsonObject.isType(Type::Object))
        return juce::StringArray();

    juce::StringArray output;

    goToPosition(jsonObject);
    readNextPosition(); //after '{'
    skipCommentsAndWhitespaces();

    int currentScope = 0;
    while (true)
    {
        if (m_currentChar == '\"') //found a key
        {
            readNextPosition();

            juce::String newKey;
            while (m_currentChar != '\"') //until end of key
            {
                newKey += m_currentChar; //get property's key
                readNextPosition();
            }
            output.add(newKey);

            readPositionAfterChar(':');
            skipCommentsAndWhitespaces();
            //should now be at the start position of property's value ('{', '\"', '1', etc.)
            switch (getTypeFromChar(m_currentChar))
            {
                case Type::Object: skipObject(true); break;
                case Type::Array: skipArray(true); break;
                case Type::String: skipString(true); break;
                case Type::Number: skipNumber(true); break;
                case Type::Bool: skipBool(true); break;
                case Type::Null: skipNull(true);  break;
                default: jassertfalse; break;
            }
        }
        else if (m_currentChar == '{')
        {
            currentScope++;
        }
        else if (m_currentChar == '}')
        {
            currentScope--;
            if (currentScope < 0)
            {
                return output; //key does not exist in current object
            }
        }
        else
            DBG("Stream::getPropertyKeys() got an unexpected char '" << m_currentChar << "' (" << ((int)m_currentChar) << ")");

        skipCommentsAndWhitespaces();
        if (m_currentChar == ',')
        {
            readNextPosition();
            skipCommentsAndWhitespaces();
        }
    }

    return output;
}

juce::Array<Property> Stream::getObjectProperties()
{
    return getObjectProperties(m_start);
}

juce::Array<Property> Stream::getObjectProperties(Position& jsonObject)
{
    juce::Array<Property> output;
    if (!jsonObject.isType(Type::Object))
        return output;

    goToPosition(jsonObject);
    readNextPosition(); //after '{'
    skipCommentsAndWhitespaces();

    int currentScope = 0;
    while (true)
    {
        if (m_currentChar == '\"') //found a key
        {
            Property newProperty(this);
            newProperty.keyReadPosition = m_readPosition;

            readNextPosition();
            while (m_currentChar != '\"')
            {
                newProperty.key += m_currentChar;
                readNextPosition();
            }
            readPositionAfterChar(':');
            skipCommentsAndWhitespaces();
            newProperty.type = getTypeFromChar(m_currentChar);
            newProperty.readPosition = m_readPosition;  //start position of property's value ('{', '\"', '1', etc.)

            switch (newProperty.type)
            {
                case Type::Object: skipObject(true); break;
                case Type::Array: skipArray(true); break;
                case Type::String: skipString(true); break;
                case Type::Number: skipNumber(true); break;
                case Type::Bool: skipBool(true); break;
                case Type::Null: skipNull(true);  break;
                default: jassertfalse; break;
            }
            output.add(newProperty);
        }
        else if (m_currentChar == '{')
        {
            currentScope++;
        }
        else if (m_currentChar == '}')
        {
            currentScope--;
            if (currentScope < 0)
                break; //end of object
        }
        else
            jassertfalse; //got an unexpected char

        skipCommentsAndWhitespaces();
        if (m_currentChar == ',')
        {
            readNextPosition();
            skipCommentsAndWhitespaces();
        }
    }

    return output;
}

int Stream::getPropertyKeyPosition(Property& jsonProperty)
{
    if (!jsonProperty)
        return -1;

    goToPosition(jsonProperty);
    readPreviousCharPosition(':');
    readPreviousPosition();
    readPreviousCharPosition('\"');
    readPreviousPosition();
    readPreviousCharPosition('\"');
    return m_readPosition;
}

Array Stream::getArray(Position& jsonProperty)
{
    if (!jsonProperty.isType(Type::Array))
        return Array();

    Array output = Array(this, jsonProperty.readPosition);
    goToPosition(jsonProperty.readPosition);
    if (m_currentChar != '[') //not an array
    {
        return output;
    }

    output.arrayStartPosition = m_readPosition;
    output.readPosition = m_readPosition;
    setNextArrayElement(output); //go to first element
    return output;
}

void Stream::setNextArrayElement(Array& fromArray)
{
    goToPosition(fromArray.readPosition);
    if (fromArray.currentElement.isNotValid()) //needs to go to first element
    {
        readNextPosition();
        skipCommentsAndWhitespaces();
    }
    else //go to next element
    {
        switch (fromArray.currentElement.type)
        {
            case Type::Object: skipObject(true); break;
            case Type::Array: skipArray(true); break;
            case Type::String: skipString(true); break;
            case Type::Number: skipNumber(true); break;
            case Type::Bool: skipBool(true); break;
            case Type::Null: skipNull(true);  break;
            default: jassertfalse; break; //type not set up
        }

        skipCommentsAndWhitespaces();
        if (m_currentChar == ',')
        {
            readNextPosition();
            skipCommentsAndWhitespaces();
        }
    }

    //should be at first/next array element ('{', '\"', '1', etc.) or end of the array
    fromArray.readPosition = m_readPosition;
    fromArray.currentElement.readPosition = m_readPosition;

    if (m_currentChar == ']') //end of array
        fromArray.currentElement.type = Type::None;
    else
        fromArray.currentElement.type = getTypeFromChar(m_currentChar);
}

int Stream::getArrayElementEndPosition(Array& jsonArray, bool afterEndChar)
{
    goToPosition(jsonArray.currentElement.readPosition);
    switch (jsonArray.currentElement.type)
    {
        case Type::Object: skipObject(afterEndChar); break;
        case Type::Array: skipArray(afterEndChar); break;
        case Type::String: skipString(afterEndChar); break;
        case Type::Number: skipNumber(afterEndChar); break;
        case Type::Bool: skipBool(afterEndChar); break;
        case Type::Null: skipNull(afterEndChar);  break;
        default: jassertfalse; break;
    }
    return m_readPosition;
}

juce::String Stream::getString(Position& jsonString)
{
    jassert(jsonString.isType(Type::String));
    goToPosition(jsonString);
    readNextPosition(); //after starting quote

    juce::String output;
    while (m_currentChar != '\"')
    {
        if (m_currentChar == '\\') //escape sequence
        {
            output += m_currentChar;
            readNextPosition();
        }
        output += m_currentChar;
        readNextPosition();
    }
    return output;
}
juce::String Stream::getString(Position& jsonString, bool applyEscapeSequences, bool ignoreHtmlText)
{
    jassert(jsonString.isType(Type::String));
    goToPosition(jsonString);
    readNextPosition(); //after starting quote

    juce::String output;
    while (m_currentChar != '\"')
    {
        if (ignoreHtmlText && m_currentChar == '<') //html rich text
        {
            while (m_currentChar != '>')
                readNextPosition();
            readNextPosition();
            continue;
        }

        if (m_currentChar == '\\') //escape sequence
        {
            if (applyEscapeSequences)
            {
                readNextPosition();
                switch (m_currentChar)
                {
                    case '"': output += '\"'; break;
                    case 'n': output += '\n'; break;
                    case 'r': output += '\r'; break;
                    case '\\': output += '\\'; break;
                    case 't': output += '\t'; break;
                    case 'f': output += '\f'; break;
                    default: output += m_currentChar; break;
                }
                readNextPosition();
                continue;
            }
            else
            {
                output += m_currentChar;
                readNextPosition();
            }
        }
        output += m_currentChar;
        readNextPosition();
    }
    return output;
}

juce::String Stream::getString()
{
    jassert(m_currentChar == '\"');
    readNextPosition(); //after starting quote

    juce::String output;
    while (m_currentChar != '\"')
    {
        output += m_currentChar;
        readNextPosition();
    }
    return output;
}

juce::String Stream::getString(const StringReadOptions& readOptions)
{
    jassert(m_currentChar == '\"');
    readNextPosition(); //after starting quote

    juce::String output;
    while (m_currentChar != '\"')
    {
        if (readOptions.skipHtmlSequences && m_currentChar == '<') //html rich text
        {
            while (m_currentChar != '>')
                readNextPosition();
            readNextPosition();
            continue;
        }

        if (m_currentChar == '\\') //escape sequence
        {
            if (readOptions.applyEsacpeSequences)
            {
                readNextPosition();
                switch (m_currentChar)
                {
                    case '"': output += '\"'; break;
                    case 'n': output += '\n'; break;
                    case 'r': output += '\r'; break;
                    case '\\': output += '\\'; break;
                    case 't': output += '\t'; break;
                    case 'f': output += '\f'; break;
                    default: output += m_currentChar; break;
                }
                readNextPosition();
                continue;
            }
            else
            {
                output += m_currentChar;
                readNextPosition();
            }
        }

        if (readOptions.handleNonUTF8)
            output += readOptions.convertToUTF8(*m_reader);
        else
            output += m_currentChar;
        readNextPosition();
    }
    return output;
}

juce::String Stream::getInt(Position& jsonInt)
{
    jassert(jsonInt.isValid());
    goToPosition(jsonInt);

    juce::String output;
    while (m_reader.isDigit() || m_currentChar == '-')
    {
        output += m_currentChar;
        readNextPosition();
    }
    return output;
}
juce::String Stream::getNumber(Position& jsonNumber)
{
    jassert(jsonNumber.isValid());
    goToPosition(jsonNumber);

    juce::String output;
    while (m_reader.isDigit() || m_currentChar == '-' || m_currentChar == '.')
    {
        output += m_currentChar;
        readNextPosition();
    }
    return output;
}
juce::String Stream::getNumber()
{
    jassert(getTypeFromChar(m_currentChar) == Type::Number);

    juce::String output;
    while (m_reader.isDigit() || m_currentChar == '-' || m_currentChar == '.')
    {
        output += m_currentChar;
        readNextPosition();
    }
    return output;
}
bool Stream::getBool(Position& jsonBool)
{
    jassert(jsonBool.isValid());
    goToPosition(jsonBool);
    return m_currentChar == 't';
}

bool Stream::getBool()
{
    jassert(m_currentChar == 't' || m_currentChar == 'f');
    return m_currentChar == 't';
}

juce::String Stream::getComment()
{
    jassert(m_currentChar == '/');

    juce::String output;

    readNextPosition();
    if (m_currentChar == '/') //single line comment
    {
        readNextPosition();
        while (m_currentChar != '\n' && m_currentChar != '\r')
        {
            output += m_currentChar;
            readNextPosition();
        }
        if (m_currentChar != '\r')
            readNextPosition();
    }
    else if (m_currentChar == '*') //multi line comment
    {
        readNextPosition();
        while (true)
        {
            if (m_currentChar == '*') //possible comment end
            {
                readNextPosition();
                if (m_currentChar == '/') //is comment end
                    break;
                else
                    readPreviousPosition(); //let it read '*'
            }

            output += m_currentChar;
            readNextPosition();
        }
    }
    readNextPosition(); //after comment end

    return output;
}

ScopeFormat Stream::getScopeFormat(bool returnToStartPosition)
{
    jassert(m_currentChar == '{' || m_currentChar == '[');

    char openChar = m_currentChar;
    char closeChar = m_currentChar == '{' ? '}' : ']';

    int startPosition = m_readPosition;
    readNextPosition();

    int currentScope = 0;
    bool isEmptyScope = true;
    while (!isEndOfJson())
    {
        if (isEmptyScope && !isAtWhitespace())
            isEmptyScope = false;

        if (m_currentChar == '\"') //skip strings
        {
            skipString(true);
        }

        if (m_currentChar == openChar)
        {
            currentScope++;
        }
        else if (m_currentChar == closeChar)
        {
            currentScope--;
            if (currentScope < 0) //end of scope, 
            {
                if (returnToStartPosition)
                    goToPosition(startPosition);

                if (!isEmptyScope)
                    return ScopeFormat::Collapsed;
                else
                    return ScopeFormat::Empty;
            }
        }
        else if (m_currentChar == '\n')
        {
            if (returnToStartPosition)
                goToPosition(startPosition);

            return ScopeFormat::Expanded; //doesn't handle the case of scope being empty and multi-lined
        }
        readNextPosition();
    }
    jassertfalse;
    return ScopeFormat::Empty;
}

void Stream::setData(int startPosition, int endPosition, const juce::String& newData, int setNewReaderPosition)
{
    m_jsonText = m_jsonText.substring(0, startPosition) + newData + m_jsonText.substring(endPosition);
    m_reader = m_jsonText.begin() + setNewReaderPosition;
    jsonResized(startPosition, endPosition - startPosition + newData.length());
}

void Stream::setString(Position& jsonProperty, const juce::String& newString)
{
    jassert(!readOnly);
    goToPosition(jsonProperty);
    jassert(getTypeFromChar(m_currentChar) == Type::String);

    int stringStartPosition = m_readPosition + 1; //frist string char
    skipString(false);
    int currentStringLength = m_readPosition - stringStartPosition;
    //==============================================================================
    // Shrink Example ('.' null termination character, 's' allocated space (0xcd)):
    // 
    //  Positions:               012345678
    //  Current m_jsonText:      "---", }.
    // 
    // stringStartPosition: 1
    // currentString: "---"
    // currentStringLength: 3
    // newString: "*"
    // newString.length(): 1
    // lengthDifference: 2
    // 
    // m_readPosition: 4
    // writePosition: 2 //where this property's value will end
    //  Positions:               012345678
    //  Current m_jsonText:      "---", }.
    //  Shrink m_jsonText:       "-", } }.
    // 
    // writePosition: 6 //new null-termination char position
    //  Positions:               012345678
    //  Current m_jsonText:      "-", } }.
    //  New Null Character:      "-", }.}.
    // 
    // writePosition: 7
    // loop(lengthDifference)
    //  Positions:               012345678
    //  Current m_jsonText:      "-", }.}.
    //  More Space:              "-", }.ss
    // 
    //==============================================================================
    // Write Example:
    // 
    // goToPosition(1) 
    // writePosition: 1
    //  Positions:               012345678
    //  Current m_jsonText:      "-", }.ss
    //  Write newString:         "*", }.ss
    //==============================================================================
    if (currentStringLength > newString.length())
    {
        //shrink json, since the new string is shorter
        int lengthDifference = currentStringLength - newString.length();
        jsonResized(stringStartPosition, lengthDifference); //notify listeners

        auto writePosition = m_reader.getAddress();
        writePosition -= lengthDifference; //where this property's value will end
        while (m_reader.isNotEmpty())
        {
            *writePosition = m_currentChar;
            writePosition++;
            readNextPosition();
        }

        *writePosition = 0; //null-termination char
        writePosition++;
        for (int i = 0; i < lengthDifference; i++)
        {
            *writePosition = '\xcd'; //convert the bytes after the string into allocated space
            writePosition++;
        }
    }
    //==============================================================================
    // Expand Example ('.' null termination character, 's' allocated space (cd)):
    // 
    //  Positions:               012345678
    //  Current m_jsonText:      "-", }.s
    // 
    // stringStartPosition: 1
    // currentString: "-"
    // currentStringLength: 1
    // newString: "***"
    // newString.length(): 3
    // 
    // currentJsonLength: 6
    // lengthDifference: 2
    // jsonTextSpaceLeft(): 1
    // preallocateBytes(8)
    //  Positions:               012345678
    //  Current m_jsonText:      "-", }.s
    //  Reallocated m_jsonText:  "-", }.ss
    // 
    // goToPosition(5)
    // writePosition: 8 //new null-termination char position
    //  Positions:               012345678
    //  Current m_jsonText:      "-", }.ss
    //  New Null Character:      "-", }.s.
    // 
    // m_readPosition: 5
    // writePosition: 7
    // lengthFromStringEndToJsonEnd: 4
    //  Positions:               012345678
    //  Current m_jsonText:      "-", }.s.
    //  Expanded m_jsonText:     "-",", }.
    // 
    //==============================================================================
    // Write Example:
    // 
    // goToPosition(1) 
    // writePosition: 1
    //  Positions:               012345678
    //  Current m_jsonText:      "-",", }.
    //  Write newString:         "***", }.
    //==============================================================================
    else if (currentStringLength < newString.length())
    {
        //expand json, since the new string is longer
        int currentJsonLength = m_jsonText.length();
        int lengthDifference = newString.length() - currentStringLength;
        jsonResized(stringStartPosition, lengthDifference);
        for (int i = 0; i < lengthDifference; i++) //expand
        {
            m_jsonText += "0";
        }
        goToJsonTextStart();

        goToPosition(currentJsonLength - 1);
        auto writePosition = m_reader.getAddress() + lengthDifference;

        int lengthFromStringEndToJsonEnd = currentJsonLength - (stringStartPosition + currentStringLength);
        for (int i = 0; i < lengthFromStringEndToJsonEnd; i++)
        {
            *writePosition = m_currentChar;
            writePosition--;
            readPreviousPosition();
        }
    }

    goToPosition(stringStartPosition);
    auto writePosition = m_reader.getAddress();
    auto newStringReader = newString.getCharPointer();
    while (newStringReader.isNotEmpty())
    {
        *writePosition = *newStringReader.getAddress();

        writePosition++;
        newStringReader++;
        readNextPosition();
    }
}
//mostly copied from setString()
void Stream::setInt(Position& jsonProperty, const juce::String& newInt)
{
    jassert(!readOnly);
    jassert(newInt.isNotEmpty());

    goToPosition(jsonProperty);
    jassert(getTypeFromChar(m_currentChar) == Type::Number);
    //readNextPosition(); //start of int value

    int stringStartPosition = m_readPosition; //first digit
    skipInt(true);
    int currentDigitCount = m_readPosition - stringStartPosition;
    if (currentDigitCount > newInt.length())
    {
        //shrink json, since the new digit count is shorter
        int lengthDifference = currentDigitCount - newInt.length();
        jsonResized(stringStartPosition, lengthDifference); //notify listeners

        auto writePosition = m_reader.getAddress();
        writePosition -= lengthDifference; //position after last digit
        while (m_reader.isNotEmpty())
        {
            *writePosition = m_currentChar;
            writePosition++;
            readNextPosition();
        }

        *writePosition = 0; //null-termination char
        writePosition++;
        for (int i = 0; i < lengthDifference; i++)
        {
            *writePosition = '\xcd'; //convert the bytes after the string into allocated space
            writePosition++;
        }
    }
    else if (currentDigitCount < newInt.length())
    {
        //expand json, since the new digit count is longer
        int currentJsonLength = m_jsonText.length();
        int lengthDifference = newInt.length() - currentDigitCount;
        jsonResized(stringStartPosition, lengthDifference);
        for (int i = 0; i < lengthDifference; i++) //expand
        {
            m_jsonText += "0";
        }
        goToJsonTextStart();

        goToPosition(currentJsonLength - 1);
        auto writePosition = m_reader.getAddress() + lengthDifference;

        int lengthFromStringEndToJsonEnd = currentJsonLength - (stringStartPosition + currentDigitCount);
        for (int i = 0; i < lengthFromStringEndToJsonEnd; i++)
        {
            *writePosition = m_currentChar;
            writePosition--;
            readPreviousPosition();
        }
    }

    goToPosition(stringStartPosition);
    auto writePosition = m_reader.getAddress();
    auto newIntReader = newInt.getCharPointer();
    while (newIntReader.isNotEmpty())
    {
        *writePosition = *newIntReader.getAddress();

        writePosition++;
        newIntReader++;
        readNextPosition();
    }
}

void Stream::flushJson() { m_jsonFile.replaceWithText(m_jsonText); }

int Stream::addResizeListener(Position* listener)
{
    if (readOnly)
        return -1;

    m_resizeListeners.add(listener);
    return m_resizeListeners.size() - 1;
}

void Stream::swapResizeListener(int indexToSwap, Position* newListener)
{
    if (readOnly || indexToSwap == -1)
        return;

    m_resizeListeners.set(indexToSwap, newListener);
}

void Stream::removeResizeListener(int listenerIndex) { m_resizeListeners.remove(listenerIndex); }

void Stream::removeResizeListener(Position* listener)
{
    for (int i = 0; i < m_resizeListeners.size(); i++)
    {
        if (m_resizeListeners[i] == listener)
            m_resizeListeners.remove(i);
    }
}

void Stream::jsonResized(int centerPosition, int shiftAmount)
{
    if (shiftAmount <= 0)
        return;

    for (auto listener : m_resizeListeners)
        listener->jsonResized(centerPosition, shiftAmount);
}

int Stream::getReadPosition() { return m_readPosition; }

char Stream::getCurrentChar() { return m_currentChar; }

bool Stream::isEndOfJson() { return m_reader.isEmpty(); }

void Stream::goToPosition(Position& jsonType)
{
    jassert(jsonType.isValid());
    int distance = jsonType.readPosition - m_readPosition;
    m_reader += distance;
    m_currentChar = *m_reader.getAddress();
    m_readPosition = jsonType.readPosition;
}

void Stream::goToPosition(int newPosition)
{
    int distance = newPosition - m_readPosition;
    m_reader += distance;
    m_currentChar = *m_reader.getAddress();
    m_readPosition = newPosition;
}

void Stream::goToJsonTextStart()
{
    m_reader = m_jsonText.getCharPointer();
    m_currentChar = *m_reader.getAddress();
    m_readPosition = 0;
}

void Stream::setJsonGridPositions(Position& forJsonType)
{
    forJsonType;
    #if CALCULATE_GRID_POSITIONS
    forJsonType.line = 1;
    forJsonType.lineChar = 1;
    juce::String::CharPointerType gridReader = m_jsonText.getCharPointer();
    int gridReadPosition = 0;
    int lastNewLinePosition = 0;
    while (gridReadPosition != m_readPosition)
    {
        if (*gridReader == '\n')
        {
            forJsonType.line++;
            lastNewLinePosition = gridReadPosition;
        }
        gridReader++;
        gridReadPosition++;
    }
    int lineStartDistance = gridReadPosition - lastNewLinePosition - 1;
    gridReader -= lineStartDistance;
    gridReadPosition -= lineStartDistance;
    while (gridReadPosition != m_readPosition)
    {
        forJsonType.lineChar++;
        gridReader++;
        gridReadPosition++;
    }
    #endif //CALCULATE_GRID_POSITIONS
}

void Stream::readLastPosition()
{
    while (m_reader.isNotEmpty())
    {
        readNextPosition();
    }
    readPreviousPosition();
}

void Stream::readNextPosition()
{
    ++m_reader;
    m_currentChar = *m_reader.getAddress();
    ++m_readPosition;

    #if CALCULATE_GRID_POSITIONS
    if (m_reader.isEmpty())
        return;
    line = 1;
    lineChar = 1;
    juce::String::CharPointerType gridReader = m_jsonText.getCharPointer();
    int gridReadPosition = 0;
    int lastNewLinePosition = 0;
    while (gridReadPosition != m_readPosition)
    {
        if (*gridReader == '\n')
        {
            line++;
            lastNewLinePosition = gridReadPosition;
        }
        gridReader++;
        gridReadPosition++;
    }
    int lineStartDistance = gridReadPosition - lastNewLinePosition - 1;
    gridReader -= lineStartDistance;
    gridReadPosition -= lineStartDistance;
    while (gridReadPosition != m_readPosition)
    {
        lineChar++;
        gridReader++;
        gridReadPosition++;
    }
    #endif
}

void Stream::readNextPosition(int increment)
{
    m_reader += increment;
    m_currentChar = *m_reader.getAddress();
    m_readPosition += increment;
}

void Stream::readPreviousPosition()
{
    --m_reader;
    m_currentChar = *m_reader.getAddress();
    --m_readPosition;
}

void Stream::readPreviousPosition(int increment)
{
    m_reader -= increment;
    m_currentChar = *m_reader.getAddress();
    m_readPosition -= increment;
}

bool Stream::trySkipWhitespaces()
{
    while (m_currentChar == ' '
           || m_currentChar == '\n'
           || m_currentChar == '\r'
           || m_currentChar == '\t'
           )
    {
        readNextPosition();
    }
    return m_reader.isNotEmpty();
}

bool Stream::isAtWhitespace()
{
    return m_currentChar == ' ' 
        || m_currentChar == '\n'
        || m_currentChar == '\r'
        || m_currentChar == '\t';
}

bool Stream::isAtSingleLineComent()
{
    if (m_currentChar != '/')
        return false;
    readNextPosition();
    bool output = m_currentChar == '/';
    readPreviousPosition();
    return output;
}

bool Stream::isAtMultiLineComent()
{
    if (m_currentChar != '/')
        return false;
    readNextPosition();
    bool output = m_currentChar == '*';
    readPreviousPosition();
    return output;
}

int Stream::readNextCharPosition(char findChar)
{
    while (m_currentChar != findChar)
    {
        readNextPosition();
    }
    return m_readPosition;
}

bool Stream::tryReadNextCharPosition(char findChar)
{
    while (m_currentChar != findChar)
    {
        if (m_reader.isEmpty())
            return false;
        readNextPosition();
    }
    return true;
}

void Stream::readPositionAfterChar(char findChar)
{
    while (m_currentChar != findChar)
    {
        readNextPosition();
    }
    readNextPosition();
}

bool Stream::tryReadPositionAfterChar(char findChar)
{
    while (m_currentChar != findChar)
    {
        if (m_reader.isEmpty())
            return false;
        readNextPosition();
    }
    readNextPosition();
    return m_reader.isNotEmpty();
}

int Stream::readPreviousCharPosition(char findChar)
{
    while (m_currentChar != findChar)
    {
        readPreviousPosition();
    }
    return m_readPosition;
}

void Stream::skipCommentsAndWhitespaces()
{
    do
    {
        skipWhitespaces();
    } while (skipComment());
}

void Stream::skipWhitespacesAndCommas()
{
    skipWhitespaces();
    if (m_currentChar == ',')
    {
        readNextPosition();
        skipWhitespacesAndCommas();
    }
}

bool Stream::skipComment()
{
    if (m_currentChar != '/') //not a comment
        return false;
    //else possible comment

    readNextPosition();
    if (m_currentChar == '/') //single line comment
    {
        while (m_currentChar != '\n')
            readNextPosition();
        readNextPosition(); //after comment
        return true;
    }
    else if (m_currentChar == '*') //multi line comment
    {
        readNextPosition();
        while (true)
        {
            readNextCharPosition('*'); //possible comment end
            readNextPosition();
            if (m_currentChar == '/') //comment end
                break;
        }
        readNextPosition(); //after comment
        return true;
    }

    //was not a comment
    readPreviousPosition();
    return false;
}

void Stream::skipWhitespaces()
{
    while (m_currentChar == ' '
           || m_currentChar == '\n'
           || m_currentChar == '\r'
           || m_currentChar == '\t'
           )
    {
        readNextPosition();
    }
}

void Stream::skipScope(bool afterEndChar)
{
    if (m_currentChar == '{')
        skipObject(afterEndChar);
    else if (m_currentChar == '[')
        skipArray(afterEndChar);
    else
        jassertfalse;
}

void Stream::skipObject(bool afterEndChar)
{
    readNextPosition();
    int currentScope = 0; //scope of current object

    while (true)
    {
        skipComment();
        if (m_currentChar == '\"') //skip strings
        {
            skipString(true);
        }

        if (m_currentChar == '{')
        {
            currentScope++;
        }
        else if (m_currentChar == '}')
        {
            currentScope--;
            if (currentScope < 0) //end of object
            {
                if (afterEndChar)
                    readNextPosition();
                return;
            }
        }
        readNextPosition();
    }
}

void Stream::skipArray(bool afterEndChar)
{
    readNextPosition();
    int currentScope = 0; //scope of current array

    while (true)
    {
        skipComment();
        if (m_currentChar == '\"') //skip strings
        {
            skipString(true);
        }

        if (m_currentChar == '[')
        {
            currentScope++;
        }
        else if (m_currentChar == ']')
        {
            currentScope--;
            if (currentScope < 0) //end of array
            {
                if (afterEndChar)
                    readNextPosition();
                return;
            }
        }
        readNextPosition();
    }
}

void Stream::skipPropertyKey(bool goToPropertyValue)
{
    readNextCharPosition(':');
    if (goToPropertyValue)
    {
        readNextPosition();
        skipCommentsAndWhitespaces();
    }
}

void Stream::skipString(bool afterEndChar)
{
    readNextPosition(); //after starting quote

    while (true)
    {
        if (m_currentChar == '\"') //end quote
        {
            if (afterEndChar)
                readNextPosition();
            return;
        }
        else if (m_currentChar == '\\') //possible quote escape sequence
        {
            readNextPosition(); //skip escape sequences
        }
        readNextPosition();
    }
}

void Stream::skipNumber(bool afterEndChar) { skipFloat(afterEndChar); }

void Stream::skipInt(bool afterEndChar)
{
    readNextPosition(); //skips first digit or negative sign

    while (m_reader.isDigit())
    {
        readNextPosition();
    }
    if (!afterEndChar)
        readPreviousPosition();
}

void Stream::skipFloat(bool afterEndChar)
{
    readNextPosition(); //skips first digit or negative sign

    while (m_reader.isDigit() || m_currentChar == '.')
    {
        readNextPosition();
    }
    if (!afterEndChar)
        readPreviousPosition();
}

void Stream::skipBool(bool afterEndChar)
{
    //"true" and "false" both end in 'e'
    readNextCharPosition('e');
    if (afterEndChar)
        readNextPosition();
}

void Stream::skipNull(bool afterEndChar)
{
    //"null" ends in 'l'
    readNextCharPosition('l');
    if (afterEndChar)
        readNextPosition();
}

Position::Position(Stream* jsonStream, int readPosition) :
    p_stream(jsonStream),
    p_resizeListenerIndex(p_stream->addResizeListener(this)),
    readPosition(readPosition)
{
}

Position::Position(Position& copy) :
    p_stream(copy.p_stream),
    p_resizeListenerIndex(p_stream->addResizeListener(this)),
    readPosition(copy.readPosition), type(copy.type)
{
    DBG("json::Position COPY");
}

Position::Position(Position&& move) noexcept :
    p_stream(move.p_stream),
    p_resizeListenerIndex(move.p_resizeListenerIndex),
    readPosition(move.readPosition), type(move.type)
{
    if (p_stream == nullptr)
        return;
    p_stream->swapResizeListener(p_resizeListenerIndex, this);
    move.p_resizeListenerIndex = -1;
}

Position::~Position()
{
    if (p_resizeListenerIndex != -1)
        p_stream->removeResizeListener(p_resizeListenerIndex);
}


Position::operator bool() const { return isValid(); }

Position Position::operator[](const juce::String& propertyKey) { return p_stream->getProperty(*this, propertyKey); }
Position Position::getProperty(const juce::String& propertyKey) { return p_stream->getProperty(*this, propertyKey); }

juce::StringArray Position::getPropertyKeys() { return p_stream->getPropertyKeys(*this); }

juce::String Position::getString() { return p_stream->getString(*this); }
juce::String Position::getString(bool applyEscapeSequences, bool ignoreHtmlText)
{
    return p_stream->getString(*this, applyEscapeSequences, ignoreHtmlText);
}
bool Position::tryGetString(juce::String& out)
{
    if (isType(Type::String))
        out = p_stream->getString(*this);
    return isType(Type::String);
}

juce::String Position::getInt() { return p_stream->getInt(*this); }
bool Position::tryGetInt(juce::String& out)
{
    if (isType(Type::Number))
    {
        out = getInt();
        return true;
    }
    return false;
}
int Position::getIntValue() { return getInt().getIntValue(); }
bool Position::tryGetIntValue(int& out)
{
    if (isType(Type::Number))
    {
        out = getIntValue();
        return true;
    }
    return false;
}

bool Position::tryGetIntValue(char& out)
{
    if (isType(Type::Number))
    {
        out = static_cast<char>(getIntValue());
        return true;
    }
    return false;
}

juce::String Position::getNumber() { return p_stream->getNumber(*this); }
bool Position::tryGetNumber(juce::String& out)
{
    if (isType(Type::Number))
    {
        out = getNumber();
        return true;
    }
    return false;
}
float Position::getFloatValue() { return getNumber().getFloatValue(); }
bool Position::tryGetFloatValue(float& out)
{
    if (isType(Type::Number))
    {
        out = getFloatValue();
        return true;
    }
    return false;
}

bool Position::getBool() { return p_stream->getBool(*this); }

bool Position::tryGetBool(bool& out)
{
    if (isType(Type::Number))
    {
        out = getBool();
        return true;
    }
    return false;
}

Array Position::getArray() { return p_stream->getArray(*this); }

void Position::setString(juce::String newString) { p_stream->setString(*this, newString); }

void Position::setInt(juce::String newInt) { p_stream->setInt(*this, newInt); }

void Position::jsonResized(int centerPosition, int shiftAmount)
{
    if (readPosition > centerPosition)
        readPosition += shiftAmount;
}

Property::Property()
{
}

Property::Property(const Property& copy) : Position(copy.p_stream, copy.readPosition), key(copy.key), keyReadPosition(copy.keyReadPosition)
{
    type = copy.type;
}

Property::Property(Stream* jsonStream, int valueReadPosition, int keyReadPosition, const juce::String& key) :
    Position(jsonStream, valueReadPosition), keyReadPosition(keyReadPosition), key(key)
{
}

Property::Property(Position& copy, const juce::String& key) :
    Position(copy), key(key)
{
}

Property::Property(Position&& move, const juce::String& key) :
    Position(std::move(move)), key(key)
{
}


Array::Array(Stream* jsonStream, int position) :
    Position(jsonStream, position), arrayStartPosition(position), 
    currentElement(Position(jsonStream, position))
{
    type = Type::Array;
}

Array::Array(Position& jsonProperty) : 
    Position(jsonProperty), arrayStartPosition(jsonProperty.readPosition),
    currentElement(jsonProperty)
{
    type = Type::Array;
}

Array::operator bool() const { return isValid() && currentElement.isValid(); }

Array Array::getArray() { return p_stream->getArray(currentElement); }

void Array::jsonResized(int centerPosition, int shiftAmount)
{
    Position::jsonResized(centerPosition, shiftAmount);
    if (arrayStartPosition > centerPosition)
        arrayStartPosition += shiftAmount;
}

bool Array::operator++() { return next(); }

bool Array::next()
{
    jassert(isValid());
    p_stream->setNextArrayElement(*this);
    currentIndex++;
    return currentElement.isValid();
}

void Array::goToStart()
{
    if (isNotValid())
        return;

    if (currentIndex == 0) //already at start
        return;

    readPosition = arrayStartPosition;
    currentIndex = 0;
    currentElement.type = Type::None;
    p_stream->setNextArrayElement(*this); //first element
}

Position& Array::operator[](int index)
{
    jassert(isValid());

    if (index >= currentIndex)
    {
        for (int i = currentIndex; i < index; i++)
        {
            next();
        }
    }
    else
    {
        goToStart();
        for (int i = 0; i < index; i++)
        {
            next();
        }
    }

    return currentElement;
}

int Array::getSize()
{
    goToStart();
    int output = 0;
    if (currentElement.isValid())
        output++;

    while (next())
        output++;

    return output;
}

juce::StringArray Array::getStrings()
{
    juce::StringArray output;
    for (auto& element : *this)
    {
        output.add(p_stream->getString(element));
    }
    return output;
}

bool Array::tryGetStrings(juce::StringArray& out)
{
    if (isValid())
    {
        out = getStrings();
        return true;
    }
    return false;
}

juce::StringArray Array::getInts()
{
    juce::StringArray output;
    for (auto& element : *this)
    {
        output.add(p_stream->getInt(element));
    }
    return output;
}

bool Array::tryGetInts(juce::StringArray& out)
{
    if (isValid())
    {
        out = getInts();
        return true;
    }
    return false;
}

int Array::getArrayElementEndPosition(bool afterEndChar) { return p_stream->getArrayElementEndPosition(*this, afterEndChar); }

Stream::StringReadOptions::StringReadOptions(bool applyEsacpeSequences, bool skipHtmlSequences, bool handleNonUTF8) :
    applyEsacpeSequences(applyEsacpeSequences),
    skipHtmlSequences(skipHtmlSequences),
    handleNonUTF8(handleNonUTF8)
{
}

void Stream::StringReadOptions::addUTF8ConversionPair(juce::juce_wchar convertFrom, char convertTo) { m_utf8ConversionSet.add({ convertFrom, convertTo }); }

char Stream::StringReadOptions::convertToUTF8(juce::juce_wchar convertCharacter) const
{
    for (auto& set : m_utf8ConversionSet)
    {
        if (set.convertFrom == convertCharacter)
            return set.convertTo;
    }
    #if 0
    if (convertCharacter > 255)
        int stop = 1;
    #endif // 0
    return static_cast<char>(convertCharacter);
}

Type getTypeFromChar(char startChar)
{
    if (startChar == '{')
    {
        return Type::Object;
    }
    else if (startChar == '[')
    {
        return Type::Array;
    }
    else if (startChar == '\"')
    {
        return Type::String;
    }
    else if ((startChar >= '0' && startChar <= '9') || startChar == '-')
    {
        return Type::Number;
    }
    else if (startChar == 'f' || startChar == 't')
    {
        return Type::Bool;
    }
    else if (startChar == 'n')
    {
        return Type::Null;
    }
    return Type::None;
}

Type getTypeFromChar(juce::juce_wchar startChar)
{
    if (startChar == '{')
    {
        return Type::Object;
    }
    else if (startChar == '[')
    {
        return Type::Array;
    }
    else if (startChar == '\"')
    {
        return Type::String;
    }
    else if ((startChar >= '0' && startChar <= '9') || startChar == '-')
    {
        return Type::Number;
    }
    else if (startChar == 'f' || startChar == 't')
    {
        return Type::Bool;
    }
    else if (startChar == 'n')
    {
        return Type::Null;
    }
    return Type::None;
}


//==============================================================================

bool Tree::isEqualJsonData(Stream& stream1, Stream& stream2, bool ignoreCommas)
{
    stream1.goToJsonTextStart();
    stream2.goToJsonTextStart();
    while (!stream1.isEndOfJson() && !stream2.isEndOfJson())
    {
        //ignore formatting such as new lines and whitespaces
        if (ignoreCommas)
        {
            stream1.skipWhitespacesAndCommas();
            stream2.skipWhitespacesAndCommas();
        }
        else
        {
            stream1.skipWhitespaces();
            stream2.skipWhitespaces();
        }
        if (stream1.getCurrentChar() != stream2.getCurrentChar())
            return false;
        if (stream1.isAtChar('"')) //strings should match exactly
        {
            int searchStart1 = stream1.getReadPosition();
            int searchStart2 = stream2.getReadPosition();
            stream1.skipString(false);
            stream2.skipString(false);
            int stringEnd1 = stream1.getReadPosition();
            int stringEnd2 = stream2.getReadPosition();

            stream1.goToPosition(searchStart1);
            stream2.goToPosition(searchStart2);
            while (stream1.getReadPosition() != stringEnd1 && stream2.getReadPosition() != stringEnd2)
            {
                if (stream1.getCurrentChar() != stream2.getCurrentChar())
                    return false; //string does not match
                stream1.readNextPosition();
                stream2.readNextPosition();
            }
            if (stream1.getReadPosition() != stringEnd1 || stream2.getReadPosition() != stringEnd2)
                return false; //string length does not match
        }
        if (stream1.isAtSingleLineComent() || stream1.isAtMultiLineComent()) //comments should match exactly
        {
            int commentStart1 = stream1.getReadPosition();
            int commentStart2 = stream2.getReadPosition();
            stream1.skipComment();
            stream2.skipComment();
            int commentEnd1 = stream1.getReadPosition() - 1;
            int commentEnd2 = stream2.getReadPosition() - 1;

            stream1.goToPosition(commentStart1);
            stream2.goToPosition(commentStart2);
            while (stream1.getReadPosition() != commentEnd1 && stream2.getReadPosition() != commentEnd2)
            {
                if (stream1.getCurrentChar() != stream2.getCurrentChar())
                    return false; //comment does not match
                stream1.readNextPosition();
                stream2.readNextPosition();
            }
            if (stream1.getReadPosition() != commentEnd1 || stream2.getReadPosition() != commentEnd2)
                return false; //comment length does not match
        }
        stream1.readNextPosition();
        stream2.readNextPosition();
    }
    stream1.skipWhitespaces();
    stream2.skipWhitespaces();
    return stream1.isEndOfJson() && stream2.isEndOfJson();
}

//==============================================================================

void Tree::initialize()
{
    if (!isValid())
        return;

    m_stream->readOnly = true;
    m_stream->goToJsonTextStart();
    initializeJson(m_base, initializeExtras(true));

    m_stream->readOnly = false;
}

void Tree::initializeJson(Entity*& currentEntity, std::vector<Extra*>& extras)
{
    if (currentEntity)
    {
        if (currentEntity->type == Type::Object)
        {
            auto currentObject = &currentEntity->toObject();
            while (m_stream->getCurrentChar() == '\"')
            {
                Property* newProperty = nullptr;
                initializeJsonProperty(newProperty, extras);
                newProperty->parent = currentObject;
                currentObject->properties.push_back(newProperty);

                extras = initializeExtras(false);
            }
            currentObject->endExtras = extras;
            m_stream->readNextPosition();
        }
        else if (currentEntity->type == Type::Array)
        {
            auto currentArray = &currentEntity->toArray();
            while (m_stream->getCurrentChar() != ']')
            {
                Entity* newElement = nullptr;
                initializeJson(newElement, extras);
                newElement->parent = currentArray;
                currentArray->elements.push_back(newElement);

                extras = initializeExtras(false);
            }
            currentArray->endExtras = extras;
            m_stream->readNextPosition();
        }
        else
            jassertfalse; //do not pass in an initialized currentEntity unless it's an Object or Array

        return;
    }

    switch (getTypeFromChar(m_stream->getCurrentChar()))
    {
        case json::Type::Object:
            currentEntity = new Object();
            currentEntity->extras = extras;

            currentEntity->toObject().scopeFormat = m_stream->getScopeFormat(true);

            m_stream->readNextPosition();
            initializeJson(currentEntity, initializeExtras(false));
            break;
        case json::Type::Array:
            currentEntity = new Array();
            currentEntity->extras = extras;

            currentEntity->toArray().scopeFormat = m_stream->getScopeFormat(true);

            m_stream->readNextPosition();
            initializeJson(currentEntity, initializeExtras(false));
            break;
        case json::Type::String:
            currentEntity = new JsonString(m_stream->getString());
            currentEntity->extras = extras;

            m_stream->readNextPosition();
            break;
        case json::Type::Number:
            currentEntity = new JsonNumber(m_stream->getNumber());
            currentEntity->extras = extras;
            break;
        case json::Type::Bool:
            currentEntity = new JsonBool(m_stream->getBool());
            currentEntity->extras = extras;

            m_stream->skipBool(true);
            break;
        case json::Type::Null:
            currentEntity = new Entity(Type::Null);
            currentEntity->extras = extras;

            m_stream->skipNull(true);
            break;
        default: jassertfalse; break;
    }
}

void Tree::initializeJsonProperty(Property*& currentProperty, std::vector<Extra*>& extras)
{
    currentProperty = new Property(m_stream->getString()); //get key

    currentProperty->extras = extras;
    m_stream->readPositionAfterChar(':');
    initializeJson(currentProperty->value, initializeExtras(false));
    currentProperty->value->parent = currentProperty;
}

std::vector<json::Tree::Extra*> Tree::initializeExtras(bool newLine)
{
    std::vector<Extra*> output;
    int emtpyLineCount = 0;
    while (!m_stream->isEndOfJson())
    {
        if (m_stream->isAtChar('/'))
        {
            if (emtpyLineCount > 0)
            {
                output.push_back(new EmptyLine(emtpyLineCount));
                emtpyLineCount = 0;
            }

            if (m_stream->isAtSingleLineComent())
            {
                output.push_back(new SingleLineComment(m_stream->getComment(), newLine));
                newLine = false;
                continue;
            }
            else if (m_stream->isAtMultiLineComent())
            {
                output.push_back(new MultiLineComment(m_stream->getComment(), newLine));
                newLine = false;
                continue;
            }
            else
                jassertfalse;
        }
        else if (m_stream->isAtChar(','))
        {
        }
        else if (!m_stream->isAtWhitespace())
        {
            if (emtpyLineCount > 0)
                output.push_back(new EmptyLine(emtpyLineCount));
            break;
        }
        else if (m_stream->isAtChar('\n'))
        {
            if (newLine)
                emtpyLineCount++;
            newLine = true;
        }
        m_stream->readNextPosition();
    }
    return output;
}

vArray<Tree::Property*> Tree::findStringProperties(const juce::String& key, const juce::String& value)
{
    vArray<Tree::Property*> output;
    findStringProperties(key, value, getBase(), output);
    return output;
}

void Tree::findStringProperties(const juce::String& key, const juce::String& value, Entity& currentEntity, vArray<Tree::Property*>& output)
{
    if (currentEntity.type == Type::Object)
    {
        for (auto& p : currentEntity.toObject().properties)
        {
            findStringProperties(key, value, *p, output);
        }
    }
    else if (currentEntity.type == Type::Property)
    {
        auto jsonProperty = &currentEntity.toProperty();
        if (jsonProperty->key == key && jsonProperty->value->type == Type::String && jsonProperty->value->toJsonString().data == value)
        {
            output.add(jsonProperty);
        }
        findStringProperties(key, value, *jsonProperty->value, output);
    }
    else if (currentEntity.type == Type::Array)
    {
        for (auto& e : currentEntity.toArray().elements)
        {
            findStringProperties(key, value, *e, output);
        }
    }
}

void Tree::createFormat(const Entity* currentEntity, juce::String& output, int currentIndent, bool indentBeforeEntity)
{
    createExtras(currentEntity->extras, output, currentIndent);
    if (indentBeforeEntity)
        output += getIndentString(currentIndent);

    if (currentEntity->type == Type::Object)
    {
        Object& currentObject = *((Object*)currentEntity);

        if (currentObject.properties.size() == 0) //empty object
        {
            createObject(currentObject, output, currentIndent, ScopeFormat::Empty);
            return;
        }
        if (currentEntity == m_base)
        {
            createObject(currentObject, output, currentIndent, ScopeFormat::Expanded);
            return;
        }

        ScopeFormat newScopeFormat = ScopeFormat::Expanded;

        //auto:
        bool hasPropertyObjects = checkIfAnyPropertyMeetsCondition(currentObject, [=](Property* p)
        {
            return p->value->type == Type::Object;
        });
        //a property's array value has more than one element or contains a nested scope
        bool hasPropertyWithBigArray = checkIfAnyPropertyMeetsCondition(currentObject, [=](Property* p)
        {
            if (p->value->type != Type::Array)
                return false;

            auto propertyArray = (Array*)p->value;
            if (propertyArray->elements.size() > 1)
                return true;

            return propertyArray->elements.size() == 1 && isScopeType(propertyArray->elements[0]->type);
        });
        if (!containsExtras(&currentObject) && currentObject.properties.size() <= 2 && !hasPropertyObjects && !hasPropertyWithBigArray)
            newScopeFormat = ScopeFormat::Collapsed;

        //custom:
        if (isOwnedByProperty(&currentObject, "ActivationEventComparators") || isOwnedByProperty(&currentObject, "ActivationEventComparator"))
            newScopeFormat = currentObject.scopeFormat; //preserve format
        else if (currentObject.parent->type == Type::Array)
        {
            auto parent2 = currentObject.parent->parent;
            bool expandObject = false;
            bool collapseObject = parent2 && parent2->type == Type::Property
                && (parent2->toProperty().key == "StoredVariableDescTokens"
                    || parent2->toProperty().key == "StoredVariableDescTokens");

            jassert(!(expandObject && collapseObject));
            if (expandObject)
                newScopeFormat = ScopeFormat::Expanded;
            else if (collapseObject)
                newScopeFormat = ScopeFormat::Collapsed;
        }

        createObject(currentObject, output, currentIndent, newScopeFormat);
    }
    else if (currentEntity->type == Type::Property)
    {
        Property& currentProperty = *((Property*)currentEntity);
        output += "\"" + currentProperty.key + "\": ";
        createFormat(currentProperty.value, output, currentIndent);
    }
    else if (currentEntity->type == Type::Array)
    {
        Array& currentArray = *((Array*)currentEntity);

        if (currentArray.elements.size() == 0) //empty array
        {
            createArray(currentArray, output, currentIndent, ScopeFormat::Empty);
            return;
        }
        if (currentEntity == m_base)
        {
            createArray(currentArray, output, currentIndent, ScopeFormat::Expanded);
            return;
        }

        ScopeFormat newScopeFormat = ScopeFormat::Expanded;

        //auto:
        if (!containsExtras(&currentArray) && !containsNestedObjects(currentArray))
            newScopeFormat = ScopeFormat::Collapsed;

        //custom:
        bool preserveFormat = isOwnedByProperty(&currentArray, "ActivationEventComparators")
            || isOwnedByProperty(&currentArray, "ActivationEventComparator");
        bool semiExpandArray = currentArray.parent->type == Type::Property && currentArray.parent->toProperty().key == "FriendlyAttackTileEffects";
        bool collapseArray = currentArray.parent->type == Type::Property && !containsExtras(&currentArray) && currentArray.elements.size() == 1
            && (currentArray.parent->toProperty().key == "AbilitySlots"
                || currentArray.parent->toProperty().key == "AbilityUpgradeStrength"
                //|| (currentArray.parent->toProperty().key == "ActivationEventComparators"
                //    && castEntity<Object>(currentArray.elements[0])->properties.size() == 1)
                );

        jassert(!(semiExpandArray && collapseArray));
        if (preserveFormat)
            newScopeFormat = currentArray.scopeFormat;
        else if (semiExpandArray)
            newScopeFormat = ScopeFormat::Expanded;
        else if (collapseArray)
            newScopeFormat = ScopeFormat::Collapsed;

        createArray(currentArray, output, currentIndent, newScopeFormat);
    }
    else
    {
        switch (currentEntity->type)
        {
            case Type::String: output += "\"" + ((JsonString*)currentEntity)->data + "\""; break;
            case Type::Number: output += ((JsonNumber*)currentEntity)->data; break;
            case Type::Bool:   output += StringFunctions::boolToString(((JsonBool*)currentEntity)->data); break;
            case Type::Null:   output += "null"; break;
            default: jassertfalse; break;
        }
    }
}

void Tree::createExtras(const std::vector<Extra*>& extras, juce::String& output, int indentCount)
{
    for (auto& e : extras)
    {
        if (e->type == ExtraType::SingleLineComment)
        {
            output += getIndentString(indentCount) + "//" + e->toSingleLineComment()->data + lineFeed;
        }
        else if (e->type == ExtraType::MultiLineComment)
        {
            output += getIndentString(indentCount) + "/*" + e->toMultiLineComment()->data + "*/" + lineFeed;
        }
        else if (e->type == ExtraType::EmptyLine)
        {
            output += getIndentString(indentCount);
            for (int i = 0; i < e->toEmptyLine()->count; i++)
                output += lineFeed;
        }
    }
}

void Tree::createObject(const Object& currentObject, juce::String& output, int currentIndent, ScopeFormat scopeFormat)
{
    switch (scopeFormat)
    {
        case ScopeFormat::Empty:
            output += '{';
            if (currentObject.endExtras.size() > 0) //semi-expand for extras
            {
                output += lineFeed;
                createExtras(currentObject.endExtras, output, ++currentIndent);
                output += getIndentString(--currentIndent) + '}';
            }
            else
                output += '}';
            break;

        case ScopeFormat::Collapsed:
            output += '{';
            for (int i = 0; i < currentObject.properties.size() - 1; i++) //prevent extra comma at the end
            {
                //dependency on array being a single-line when it doesn't contain nested objects
                createFormat(currentObject.properties[i], output, currentIndent);
                output += ", ";
            }
            createFormat(currentObject.properties[currentObject.properties.size() - 1], output, currentIndent);
            output += '}';
            break;

        case ScopeFormat::Expanded:
            output += "{";
            output += lineFeed;
            currentIndent++;
            for (int i = 0; i < currentObject.properties.size() - 1; i++) //prevent extra comma at the end
            {
                createFormat(currentObject.properties[i], output, currentIndent, true);
                output += ",";
                output += lineFeed;
            }
            createFormat(currentObject.properties[currentObject.properties.size() - 1], output, currentIndent, true);

            output += lineFeed;
            createExtras(currentObject.endExtras, output, currentIndent);

            output += getIndentString(--currentIndent) + '}';
            break;

        default: jassertfalse; break;
    }
}


void Tree::createArray(const Array& currentArray, juce::String& output, int currentIndent, ScopeFormat scopeFormat)
{
    switch (scopeFormat)
    {
        case ScopeFormat::Empty:
            output += '[';
            if (currentArray.endExtras.size() > 0) //semi-expand for extras
            {
                output += lineFeed;
                createExtras(currentArray.endExtras, output, ++currentIndent);
                output += getIndentString(--currentIndent) + ']';
            }
            else
                output += ']';
            break;

        case ScopeFormat::Collapsed:
            output += '[';
            for (int i = 0; i < currentArray.elements.size() - 1; i++) //prevent extra comma at the end
            {
                createFormat(currentArray.elements[i], output, 0);
                output += ", ";
            }
            createFormat(currentArray.elements[currentArray.elements.size() - 1], output, 0);
            output += ']';
            break;
        case ScopeFormat::Expanded:
            output += "[";
            output += lineFeed;
            currentIndent++;
            for (int i = 0; i < currentArray.elements.size() - 1; i++) //prevent extra comma at the end
            {
                createFormat(currentArray.elements[i], output, currentIndent, true);
                output += ",";
                output += lineFeed;
            }
            createFormat(currentArray.elements[currentArray.elements.size() - 1], output, currentIndent, true);

            output += lineFeed;
            createExtras(currentArray.endExtras, output, currentIndent);

            output += getIndentString(--currentIndent) + ']';
            break;

        default: jassertfalse; break;
    }
}

bool Tree::containsComments(const Entity* entity)
{
    if (containsComments(entity->extras))
        return true;

    if (entity->type == Type::Array)
    {
        if (containsComments(entity->toArray().endExtras))
            return true;

        for (auto& e : ((Array*)entity)->elements)
        {
            if (containsComments(e))
                return true;
        }
    }
    else if (entity->type == Type::Object)
    {
        if (containsComments(entity->toObject().endExtras))
            return true;

        for (auto& p : ((Object*)entity)->properties)
        {
            if (containsComments(p))
                return true;
        }
    }
    return false;
}

bool Tree::containsComments(const std::vector<Extra*>& extras)
{
    for (auto& e : extras)
        if (e->type == ExtraType::SingleLineComment || e->type == ExtraType::MultiLineComment)
            return true;
    return false;
}

bool Tree::containsExtras(const Object* objectEntity)
{
    if (objectEntity->endExtras.size() > 0)
        return true;
    for (auto& p : objectEntity->properties)
    {
        if (p->extras.size() > 0)
            return true;
        if (containsExtras(p->value))
            return true;
    }
    return false;
}

bool Tree::containsExtras(const Array* arrayEntity)
{
    if (arrayEntity->endExtras.size() > 0)
        return true;
    for (auto& e : arrayEntity->elements)
    {
        if (containsExtras(e))
            return true;
    }
    return false;
}
bool Tree::containsExtras(const Entity* entity)
{
    if (entity->extras.size() > 0)
        return true;

    if (entity->type == Type::Array)
    {
        if (entity->toArray().endExtras.size() > 0)
            return true;

        for (auto& e : ((Array*)entity)->elements)
        {
            if (containsExtras(e))
                return true;
        }
    }
    else if (entity->type == Type::Object)
    {
        if (entity->toObject().endExtras.size() > 0)
            return true;

        for (auto& p : ((Object*)entity)->properties)
        {
            if (p->extras.size() > 0)
                return true;
            if (containsExtras(p->value))
                return true;
        }
    }
    return false;
}

bool Tree::containsNestedScopes(const Object& checkScope)
{
    for (auto& property : checkScope.properties)
        if (isScopeType(property->value->type))
            return true;
    return false;
}

bool Tree::containsNestedScopes(const Array& checkScope)
{
    for (auto& element : checkScope.elements)
        if (isScopeType(element->type))
            return true;
    return false;
}

bool Tree::checkIfAnyPropertyMeetsCondition(Object& checkObject, std::function<bool(Property*)> condition)
{
    for (auto& property : checkObject.properties)
    {
        if (condition(property))
            return true;
    }
    return false;
}

bool Tree::containsNestedObjects(const Array& checkScope)
{
    for (auto element : checkScope.elements)
    {
        if (element->type == Type::Object)
            return true;
        if (element->type == Type::Array && containsNestedObjects(*(Array*)element))
            return true;
    }
    return false;
}

juce::String Tree::getIndentString(int indentCount)
{
    juce::String output;
    for (int i = 0; i < indentCount; i++)
        output += '\t';
    return output;
}

bool Tree::isOwnedByProperty(const Entity* child, juce::String propertyKey)
{
    Entity* currentParent = child->parent;
    while (currentParent != nullptr)
    {
        if (currentParent->type == Type::Property && currentParent->toProperty().key == propertyKey)
            return true;
        currentParent = currentParent->parent;
    }
    return false;
}

} //namespace json