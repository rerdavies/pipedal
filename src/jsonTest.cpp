// Copyright (c) 2022 Robin Davies
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "pch.h"
#include "catch.hpp"
#include <sstream>
#include <cstdint>
#include <string>


#include "json.hpp"

using namespace pipedal;

class JsonTestTarget {
private:
    JsonTestTarget(const JsonTestTarget&) {} // hide copy constructor.
    JsonTestTarget& operator=(const JsonTestTarget&) { return *this;} // hide assignment.
public: 
    JsonTestTarget() // make sure reading works without  a default cosntructor.
    {
    }
    bool operator==(const JsonTestTarget &other) const
    {
        return this->int_ == other.int_
        && this->float_ == other.float_
        && this->double_ == other.double_
        && this->string_ == other.string_
        && std::equal(this->ints_.begin(),this->ints_.end(), other.ints_.begin(),other.ints_.end())
        && std::equal(this->strings_.begin(),this->strings_.end(), other.strings_.begin(),other.strings_.end())
        ;

    }
public:
    int int_ = 1;
    float float_ = 3;
    double double_ = 4;
    
public:
    std::string string_{"5"};

    std::vector<int> ints_ { 1,2,3,4,5};
    std::vector<std::string> strings_ { "a","b","c","d"};
    std::vector<std::vector<int> > compound_array_ { { 1,2},{3},{4,5,6},{9}};

public:
    static json_map::storage_type<JsonTestTarget> jmap;


public:
    JsonTestTarget(json_reader& reader)
    {
        #ifdef JUNK
        while (true)
        {
            const char*name = reader.read_member_name();
            if (name == nullptr) 
            {
                break;
            }
        }
        #endif
    }

};

json_map::storage_type<JsonTestTarget> JsonTestTarget::jmap {{
    //json_map::reference("char_", &JsonTestTarget::char_),
    json_map::reference("int", &JsonTestTarget::int_),
    json_map::reference("floatt", &JsonTestTarget::float_),
    json_map::reference("double", &JsonTestTarget::double_),
    json_map::reference("ints", &JsonTestTarget::ints_),
    json_map::reference("string", &JsonTestTarget::string_),
    json_map::reference("strings", &JsonTestTarget::strings_),
    json_map::reference("compoundarray", &JsonTestTarget::compound_array_),
}};



TEST_CASE( "json write", "[json_write_test]" ) {
    std::stringstream os;
    json_writer writer { os };

    JsonTestTarget testTarget;

    writer.write(testTarget);

    std::cout << os.str();
}

static std::string get_json()
{
    std::stringstream os;
    json_writer writer { os };

    JsonTestTarget testTarget;

    writer.write(testTarget);
    return os.str();

}


TEST_CASE( "json read", "[json_read_test]" ) {

    JsonTestTarget source;
    source.ints_ = { 1,7,4};
    source.string_ = "xyz";
    source.double_ = 99483.1837;

    std::stringstream os;
    json_writer writer(os);
    writer.write(source);
    std::string json = os.str();

    std::stringstream input (json);
    json_reader reader { input };

    JsonTestTarget dest;
    reader.read(&dest);
    REQUIRE(reader.is_complete());

    REQUIRE( source == dest);
    
    
    JsonTestTarget *pDest;
    std::stringstream input2(json);
    json_reader reader2 { input2};
    reader2.read(&pDest);
    
    
    //JsonTestTarget *testTarget = reader.read_object<JsonTestTarget>();

}

TEST_CASE( "json smart ptrs", "[json_smart_ptrs]" ) {
    std::string json =get_json();


    {
        std::unique_ptr<JsonTestTarget> uniquePtr;
        std::stringstream input(json);
        json_reader reader { input };
        reader.read(&uniquePtr); 
    }
    {
        std::shared_ptr<JsonTestTarget> sharedPtr;
        std::stringstream input(json);
        json_reader reader { input };
        reader.read(&sharedPtr); 
    }

}