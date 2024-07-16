#ifndef XML_H
#define XML_H

#include <string>
#include <map>
#include <list>
#include <set>
#include <stack>
#include <vector>
#include <memory>
#include <fstream>

//Need Parser
//Need <? .... ?>
//?Need !?types
//Need comment
//Need parent
//Need (Up <- tree search -> Down, All)
//set recursive depth tree

class XmlBufferReader
{
public:
    explicit XmlBufferReader(){}
    virtual ~XmlBufferReader(){}

    virtual bool next() = 0;
    virtual unsigned char value() = 0;
    virtual std::size_t offset() = 0;
};

class XmlStringViewBufferReader : public XmlBufferReader
{
    long long pos = -1;
    std::string_view _xml;

public:
    explicit XmlStringViewBufferReader(std::string_view xml);
    bool next() override;
    unsigned char value() override;
    std::size_t offset() override;
};

class XmlFileBufferReader : public XmlBufferReader
{
    char c = 0;
    std::size_t pos = 0;
    std::ifstream stream;
    bool is_open = false;

public:
    explicit XmlFileBufferReader();
    bool open(const std::string & fileName);
    bool isOpen();
    bool next() override;
    unsigned char value() override;
    std::size_t offset() override;
};

#include <iostream>

class XmlSAXReader
{
    std::string _error;
    bool stop;

protected:
    void stopParse();

public:

    enum Operation : unsigned char
    {
        Single,
        Multiple
    };

    explicit XmlSAXReader();
    virtual ~XmlSAXReader();

    std::string error() const;
    bool parse(XmlBufferReader & buffer, Operation operation);

    void XmlBegin(){ std::cout << "Xml begin" << std::endl;}
    void XmlEnd(){ std::cout << "Xml end" << std::endl;}

    void NodeBegin(const std::string & name){ std::cout << "Node: " << name << std::endl; }
    void AttributeName(const std::string & name){  std::cout << "Attribute Name: " << name << std::endl;  }
    void AttributeValue(const std::string & value){  std::cout << "Attribute Value: " << value << std::endl;  }
    void Value(const std::string & value){  std::cout << "NodeValue: " << value << std::endl;  }
    void NodeEnd(){ std::cout << "NodeEnd: " << std::endl; }
};

class XmlNode final
{
    friend class XmlWriter;
public:
    using Attributes = std::map<std::string, std::string>;
    using Childs = std::list<XmlNode>;

private:
    struct XmlData
    { 
       bool sort = false;
       std::string name, value;
       Attributes attributes;
       Childs childs;
    };

    std::shared_ptr<XmlData> data = std::make_shared<XmlData>();

public:
    explicit XmlNode();
    explicit XmlNode(const std::string & nodeName, bool sort = true);

    bool isValid() const;
    const std::string & nodeName() const;

    XmlNode copy() const;

    std::size_t attributesCount() const;
    Attributes & attributes();
    const Attributes & attributes() const;
    operator Attributes & ();
    operator const Attributes & () const;
    void setAttributes(const Attributes & attributes);
    bool containsAttribute(const std::string & attributeName) const;
    std::string attributeValue(const std::string & attributeName) const;
    void addAttribute(const std::string & attributeName, const std::string & value);
    void removeAttribute(const std::string & attributeName);
    void clearAttributes();

    bool isValue() const;
    const std::string & value() const;
    operator const std::string &() const;
    void setValue(const char * value);
    void setValue(std::string_view value);
    void setValue(const std::string & value);

    bool isChilds() const;
    std::size_t childsCount() const;
    const Childs & childs() const;
    operator const Childs & () const;
    void setChilds(const Childs & childs);
    bool containsChild(const std::string & nodeName) const;
    std::vector<XmlNode> child(const std::string & nodeName) const;
    bool addChild(const XmlNode & node);
    void removeChild(const std::string & nodeName);

    bool operator<(const XmlNode & other) const;
};

class XmlBufferWriter
{
public:
    explicit XmlBufferWriter(){}
    virtual ~XmlBufferWriter(){}

    virtual bool write(unsigned char ch) = 0;
    virtual std::size_t writeCount() = 0;
};

class XmlStringBufferWriter : public XmlBufferWriter
{
    std::size_t count = 0;
    std::string xml;

public:
    explicit XmlStringBufferWriter();
    bool write(unsigned char ch) override;
    std::size_t writeCount() override;
    const std::string & result() const;
};

class XmlFileBufferWriter : public XmlBufferWriter
{
    std::size_t count = 0;
    std::ofstream stream;
    bool is_open = false;

public:
    explicit XmlFileBufferWriter();
    bool open(const std::string & fileName);
    bool isOpen();
    bool write(unsigned char ch) override;
    std::size_t writeCount() override;
};

class XmlSAXWriter
{
    bool beautiful = false;
    std::string _error;
    XmlBufferWriter * buffer = nullptr;

    enum class Сondition : unsigned char
    {
        NodeBegin,
        NodeAttribute,
        NodeValue,
        NodeChilds,
        NodeEnd
    };

    std::stack<std::tuple<Сondition, std::string, std::set<std::string>>> stack;

    bool checkBuffer();
    bool writeChar(unsigned char ch);
    bool writeSpace(int count);
    bool writeName(const std::string & name);
    bool writeString(std::string_view string);
    bool writeValue(const std::string & value, bool isAttrebute);

public:
    explicit XmlSAXWriter();
    std::string error() const;
    void setBuffer(XmlBufferWriter * buffer, bool beautiful = false);

    bool NodeBegin(const std::string & name);
    bool AttributeName(const std::string & name);
    bool AttributeValue(const std::string & value);
    bool Value(const std::string & value);
    bool NodeEnd();
};

class XmlWriter final : public XmlSAXWriter
{
public:
    explicit XmlWriter();
    bool write(XmlBufferWriter & buffer, const XmlNode & node, bool beautiful = false);
    bool write(std::string & string, const XmlNode & json, bool beautiful = false);
    std::string write(const XmlNode & json, bool beautiful = false);
    bool writeToFile(const std::string & fileName, const XmlNode & node, bool beautiful = false);
};

#endif // XML_H
