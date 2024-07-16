#include "Xml.h"

static inline bool isControlCode(unsigned char value){ return (value <= 8 || (value >= 14 && value <= 31) || value == 127); }

//-------------------------------------------------------------------------------------------

XmlStringViewBufferReader::XmlStringViewBufferReader(std::string_view xml):XmlBufferReader(), _xml(xml){}

bool XmlStringViewBufferReader::next()
{
    pos++;
    if(static_cast<std::size_t>(pos) == _xml.size()) return false;
    return true;
}

unsigned char XmlStringViewBufferReader::value(){ return (static_cast<std::size_t>(pos) == _xml.size()) ? 0 : _xml[pos]; }

std::size_t XmlStringViewBufferReader::offset(){ return static_cast<std::size_t>(pos); }

//---------------

XmlFileBufferReader::XmlFileBufferReader(){}

bool XmlFileBufferReader::open(const std::string & fileName)
{
    c = 0;
    pos = 0;
    if(stream.is_open()) stream.close();
    stream.open(fileName);
    is_open = stream.is_open();
    return is_open;
}

bool XmlFileBufferReader::isOpen(){ return is_open; }

bool XmlFileBufferReader::next()
{
    if(stream.get(c))
    {
       pos++;
       return true;
    }

    return false;
}

unsigned char XmlFileBufferReader::value(){ return c; }

std::size_t XmlFileBufferReader::offset(){ return pos; }

//-------------------------------------------------------------------------------------------

static const char * const ControlCharacterDetectionMsg = "Control character detection, offset: ",
                  * const InvalidEntryCharacterMsg = "Invalid entry character '";

static std::string makeError(const char * msg, XmlBufferReader & buffer)
{
    return  msg + std::to_string(buffer.offset());
}

static std::string makeError(const char * msg, unsigned char ch, XmlBufferReader & buffer)
{
    return std::string(msg) + static_cast<const char>(ch) + "', offset: " + std::to_string(buffer.offset());
}

//-------------------------------------------------------------------------------------------

enum class XmlReaderType : unsigned char
{
    NodeBegin = 0,
    NodeAttribute,
    NodeAttributeValue,
    NodeValue,
    NodeEnd
};

static bool readyNodeName(std::stack<XmlReaderType> & depth, XmlSAXReader * self, XmlBufferReader & buffer, std::string & error)
{
    int i = 0;
    bool exit = false;
    std::string name;

    unsigned char ch;

    while(buffer.next())
    {
          ch = buffer.value();

          if(isControlCode(ch))
          {
             error =  makeError(ControlCharacterDetectionMsg, buffer);
             return false;
          }

          if(std::isspace(ch) != 0)
          {
             if(i > 0)
             {
                self->NodeBegin(name);
                depth.push(XmlReaderType::NodeBegin);
                return true;
             }
             else
             {
                error = "";
                return false;
             }
          }

          if(ch == '/')
          {
             if(i > 0)
             {
                if(!buffer.next()) break;

                if(buffer.value() != '>')
                {
                   error = "";
                   return false;
                }

                self->NodeBegin(name);
                self->NodeEnd();
                return true;
             }
             else
             {
                error = "";
                return false;
             }
          }

          if(ch == '>')
          {
             if(i > 0)
             {
                self->NodeBegin(name);
                depth.push(XmlReaderType::NodeValue);
                return true;
             }
             else
             {
                error = "";
                return false;
             }
          }

          if((i == 0 && std::isalpha(ch) == 0) || (std::isalpha(ch) == 0 && ch != ':' && ch != '-' && ch != '_' && ch != '.'))
          {
             error = "";
             return false;
          }

          name.push_back(ch);

          i++;
    }

    if(!exit)
    {
       error = "";
       return false;
    }

    return true;
}

//----------------------------------------------------------------

void XmlSAXReader::stopParse(){ stop = true; }

XmlSAXReader::XmlSAXReader(){}
XmlSAXReader::~XmlSAXReader(){}

std::string XmlSAXReader::error() const{ return std::move(_error); }

bool XmlSAXReader::parse(XmlBufferReader & buffer, Operation operation)
{
    stop = false;
    std::stack<XmlReaderType> depth;

    while(buffer.next())
    {
        const unsigned char ch = buffer.value();

        if(std::isspace(ch) != 0) continue;

        if(isControlCode(ch))
        {
           _error =  makeError(ControlCharacterDetectionMsg, buffer);
           return false;
        }

        if(depth.empty())
        {
           if(ch == '<')
           {
              XmlBegin();
              if(!readyNodeName(depth,this, buffer, this->_error)) return false;
              continue;
           }
           else
           {
              _error =  makeError(InvalidEntryCharacterMsg, ch, buffer);
              return false;
           }
        }
        else if(ch == '<')
        {
           if(depth.top() == XmlReaderType::NodeValue)
           {
              if(!readyNodeName(depth,this, buffer, this->_error)) return false;
              continue;
           }
           else
           {
              _error = "";
           }
        }
        else if(ch == '>')
        {
           if(depth.top() == XmlReaderType::NodeBegin)
           {

           }
           else
           {
              _error = "";
           }
        }
        else if(ch == '/')
        {
           if(depth.top() == XmlReaderType::NodeBegin)
           {

           }
           else
           {
              _error = "";
           }
        }
        else if(ch == '=')
        {
           if(depth.top() == XmlReaderType::NodeAttribute)
           {

           }
           else
           {
              _error = "";
           }
        }
        else if(ch == '\'' || ch == '"')
        {
           if(depth.top() == XmlReaderType::NodeAttributeValue)
           {

           }
           else
           {
              _error = "";
           }
        }
        else
        {
           if(depth.top() == XmlReaderType::NodeBegin)
           {

           }
           else
           {
              _error = "";
           }
        }

        ///next xml
    }

    if(!depth.empty())
    {
       _error = "";
       return false;
    }

    return true;
}

//-------------------------------------------------------------------------------------------

XmlNode::XmlNode(){}
XmlNode::XmlNode(const std::string & nodeName, bool sort)
{
    data->sort = sort;
    data->name = nodeName;
}

bool XmlNode::isValid() const { return !data->name.empty(); }
XmlNode XmlNode::copy() const
{
    XmlNode ret;
    *ret.data = *data;
    return ret;
}
std::size_t XmlNode::attributesCount() const { return data->attributes.size(); }
XmlNode::Attributes & XmlNode::attributes(){ return data->attributes; }
const XmlNode::Attributes & XmlNode::attributes() const { return data->attributes; }
XmlNode::operator Attributes & (){ return data->attributes; }
XmlNode::operator const Attributes & () const{ return data->attributes; }
void XmlNode::setAttributes(const Attributes & attributes){ data->attributes = attributes; }
bool XmlNode::containsAttribute(const std::string & attributeName) const{ return data->attributes.contains(attributeName); }
std::string XmlNode::attributeValue(const std::string & attributeName) const { return (data->attributes.contains(attributeName)) ? data->attributes[attributeName] : std::string(); }
void XmlNode::addAttribute(const std::string & attributeName, const std::string & value){ data->attributes[attributeName] = value; }
void XmlNode::removeAttribute(const std::string & attributeName){ data->attributes.erase(attributeName); }
void XmlNode::clearAttributes(){ data->attributes.clear(); }

bool XmlNode::isValue() const { return !data->value.empty(); }
const std::string & XmlNode::value() const { return data->value; }
XmlNode::operator const std::string &() const { return data->value; }
void XmlNode::setValue(const char * value)
{
    data->childs.clear();
    data->value = value;
}
void XmlNode::setValue(std::string_view value)
{
    data->childs.clear();
    data->value = value;
}
void XmlNode::setValue(const std::string & value)
{
    data->childs.clear();
    data->value = value;
}
bool XmlNode::isChilds() const { return !data->childs.empty(); }
std::size_t XmlNode::childsCount() const { return data->childs.size(); }
const XmlNode::Childs & XmlNode::childs() const { return data->childs; }
XmlNode::operator const Childs & () const { return data->childs; }
void XmlNode::setChilds(const Childs & childs)
{
    data->value.clear();
    data->childs = childs;
    if(data->sort) data->childs.sort();
}
bool XmlNode::containsChild(const std::string & nodeName) const
{
    for(const auto & child : data->childs){ if(child.data->name == nodeName) return true; }
    return false;
}
std::vector<XmlNode> XmlNode::child(const std::string & nodeName) const
{
    std::vector<XmlNode> ret;
    for(const auto & child : data->childs){ if(child.data->name == nodeName) ret.push_back(child); }
    return ret;
}
bool XmlNode::addChild(const XmlNode & node)
{
    if(!node.isValid()) return false;
    data->value.clear();
    data->childs.push_back(node);
    if(data->sort) data->childs.sort();
    return true;
}
void XmlNode::removeChild(const std::string & nodeName)
{
    std::size_t count = 0;
    for(auto begin = data->childs.begin(); begin != data->childs.end(); begin++)
    {
        if(begin->data->name == nodeName)
        {
           begin = data->childs.erase(begin);
           count++;
        }
    }

    if(count > 0 && data->sort) data->childs.sort();
}

bool XmlNode::operator<(const XmlNode & other) const{ return (data->name < other.data->name); }

//-----------------------------------------------------------

XmlStringBufferWriter::XmlStringBufferWriter(){}

bool XmlStringBufferWriter::write(unsigned char ch)
{
    count++;
    xml.push_back(ch);
    return true;
}

std::size_t XmlStringBufferWriter::writeCount(){ return count; }

const std::string & XmlStringBufferWriter::result() const { return xml; }

//------------------

XmlFileBufferWriter::XmlFileBufferWriter(){}

bool XmlFileBufferWriter::open(const std::string &fileName)
{
    count = 0;
    if(stream.is_open()) stream.close();
    stream.open(fileName);
    is_open = stream.is_open();
    return is_open;
}

bool XmlFileBufferWriter::isOpen(){ return is_open; }

bool XmlFileBufferWriter::write(unsigned char ch)
{
    if(!is_open) return false;
    stream << ch;
    count++;
    return true;
}

std::size_t XmlFileBufferWriter::writeCount(){ return count; };

//-----------------------------------------------------------

static const char * const InvalidBuffer = "Invalid buffer",
                  * const InvalidOperation = "Invalid operation",
                  * const InvalidName = "Invalid name",
                  * const ControlCharacterDetect = "Control character detection",
                  * const BufferEnding = "Buffer ending";

bool XmlSAXWriter::checkBuffer()
{
    if(buffer == nullptr)
    {
       _error = InvalidBuffer;
       return false;
    }

    return true;
}

bool XmlSAXWriter::writeChar(unsigned char ch)
{
    if(isControlCode(ch))
    {
       _error = ControlCharacterDetect;
       return false;
    }

    if(!buffer->write(ch))
    {
       _error = BufferEnding;
       return false;
    }

    return true;
}

bool XmlSAXWriter::writeSpace(int count)
{
    for(int i = 0; i < count; i++){ if(!writeChar(' ')) return false; }
    return true;
}

bool XmlSAXWriter::writeName(const std::string & name)
{
    int i = 0;

    if(name.size() >= 3)
    {
       const char * const xml = "xml";

       for(; i < 3; i++)
       {
           if(xml[i] != std::tolower(name[i])) break;
       }
    }

    if(i == 3)
    {
       _error = InvalidName;
       return false;
    }

    i = 0;

    for(unsigned char c : name)
    {
        if(std::isspace(c) != 0 || (i == 0 && std::isalpha(c) == 0) || (std::isalpha(c) == 0 && c != ':' && c != '-' && c != '_' && c != '.'))
        {
           _error = InvalidName;
        }

        if(!writeChar(c)) return false;
        i++;
    }

    return true;
}

bool XmlSAXWriter::writeString(std::string_view string)
{
    for(unsigned char c : string){ if(!writeChar(c)) return false; }
    return true;
}

bool XmlSAXWriter::writeValue(const std::string & value, bool isAttrebute)
{
    if(isAttrebute && !writeChar('"')) return false;

    for(unsigned char c : value)
    {
        switch (c)
        {
           case '<': if(!writeString("&lt;")) return false;
           break;
           case '>': if(!writeString("&gt;")) return false;
           break;
           case '&': if(!writeString("&amp;")) return false;
           break;
           case '"': if(!writeString("&quot;")) return false;
           break;
           case '\'': if(!writeString("&apos;")) return false;
           break;
           default: if(!writeChar(c)) return false;
        }
    }

    if(isAttrebute && !writeChar('"')) return false;
    return true;
}

XmlSAXWriter::XmlSAXWriter(){}

std::string XmlSAXWriter::error() const { return std::move(_error); }

void XmlSAXWriter::setBuffer(XmlBufferWriter * buffer, bool beautiful)
{
    while(!stack.empty()) stack.pop();
    this->buffer = buffer;
    this->beautiful = beautiful;
}

bool XmlSAXWriter::NodeBegin(const std::string & name)
{
    if(!stack.empty())
    {
       if(std::get<0>(stack.top()) == Сondition::NodeValue || std::get<0>(stack.top()) == Сondition::NodeAttribute)
       {
          _error = InvalidOperation;
          return false;
       }

       if(std::get<0>(stack.top()) == Сondition::NodeBegin)
       { 
          if(!writeChar('>') || (beautiful && !writeChar('\n'))) return false;
          std::get<0>(stack.top()) = Сondition::NodeChilds;
       }

       if(beautiful){ if(!writeSpace(4 * stack.size())) return false; }
    }

    if(!writeChar('<') || !writeName(name)) return false;
    stack.push({Сondition::NodeBegin, std::string(), std::set<std::string>()});
    std::get<1>(stack.top()) = name;

    return true;
}

bool XmlSAXWriter::AttributeName(const std::string & name)
{
    if(stack.empty() || std::get<0>(stack.top()) != Сondition::NodeBegin || std::get<2>(stack.top()).contains(name))
    {
       _error = InvalidOperation;
       return false;
    }

    if(!writeChar(' ') || !writeName(name)) return false;
    std::get<2>(stack.top()).insert(name);
    std::get<0>(stack.top()) = Сondition::NodeAttribute;
    return true;
}

bool XmlSAXWriter::AttributeValue(const std::string & value)
{
    if(stack.empty() || (std::get<0>(stack.top()) != Сondition::NodeAttribute))
    {
       _error = InvalidOperation;
       return false;
    }

    if(!writeChar('=') || !writeValue(value, true)) return false;
    std::get<0>(stack.top()) = Сondition::NodeBegin;
    return true;
}

bool XmlSAXWriter::Value(const std::string & value)
{
    if(stack.empty() || (std::get<0>(stack.top()) != Сondition::NodeBegin))
    {
       _error = InvalidOperation;
       return false;
    }

    if(!writeChar('>') || !writeValue(value, false)) return false;
    std::get<0>(stack.top()) = Сondition::NodeValue;
    return true;
}

bool XmlSAXWriter::NodeEnd()
{
    if(stack.empty() || (std::get<0>(stack.top()) == Сondition::NodeAttribute || std::get<0>(stack.top()) == Сondition::NodeChilds))
    {
       _error = InvalidOperation;
       return false;
    }

    if(std::get<0>(stack.top()) == Сondition::NodeBegin && (!writeChar('/') || !writeChar('>')))
    {
       return false;
    }
    else if(std::get<0>(stack.top()) == Сondition::NodeValue || std::get<0>(stack.top()) == Сondition::NodeEnd)
    {
       if(beautiful && std::get<0>(stack.top()) == Сondition::NodeEnd && !writeSpace(4 * (stack.size() - 1))) return false;
       if(!writeChar('<') || !writeChar('/') || !writeName(std::get<1>(stack.top())) || !writeChar('>')) return false;
    }

    if(beautiful && !writeChar('\n')) return false;

    stack.pop();
    if(!stack.empty()) std::get<0>(stack.top()) = Сondition::NodeEnd;
    return true;
}

//-----------------------------------------------------------

XmlWriter::XmlWriter(){}

bool XmlWriter::write(XmlBufferWriter & buffer, const XmlNode & node, bool beautiful)
{
    if(node.data->name.empty()) return false;
    using ChildIter = XmlNode::Childs::iterator;
    std::list<std::tuple<XmlNode::XmlData *, bool, ChildIter>> stack;
    stack.push_back({node.data.get(), false, node.data->childs.begin()});
    setBuffer(&buffer, beautiful);

    while(!stack.empty())
    {
          if(!std::get<1>(stack.back()))
          {
             if(!NodeBegin(std::get<0>(stack.back())->name)) return false;

             for(const auto & pair : std::get<0>(stack.back())->attributes)
             {
                 if(!AttributeName(pair.first)) return false;
                 if(!AttributeValue(pair.second)) return false;
             }

             if(!std::get<0>(stack.back())->value.empty())
             {
                if(!Value(std::get<0>(stack.back())->value)) return false;
                if(!NodeEnd()) return false;
                stack.pop_back();
                continue;
             }

             std::get<1>(stack.back()) = true;
          }

          if(std::get<2>(stack.back()) != std::get<0>(stack.back())->childs.end())
          {
             ChildIter iter = std::get<2>(stack.back())++;
             int count = 0;

             for(const auto & tuple : stack)
             {
                 if(std::get<0>(tuple) == iter->data.get())
                 {
                    count++;
                    break;
                 }
             }

             if(count == 0) stack.push_back({iter->data.get(), false, iter->data->childs.begin()});
             continue;
          }

          if(!NodeEnd()) return false;
          stack.pop_back();
    }

    return true;
}

bool XmlWriter::write(std::string & string, const XmlNode & node, bool beautiful)
{
    XmlStringBufferWriter buffer;
    if(!write(buffer, node, beautiful)) return false;
    string = std::move(const_cast<std::string &>(buffer.result()));
    return true;
}

std::string XmlWriter::write(const XmlNode & node, bool beautiful)
{
    std::string ret;
    write(ret, node, beautiful);
    return ret;
}

bool XmlWriter::writeToFile(const std::string & fileName, const XmlNode &node, bool beautiful)
{
    XmlFileBufferWriter buffer;
    if(!buffer.open(fileName) || !write(buffer, node, beautiful)) return false;
    return true;
}
