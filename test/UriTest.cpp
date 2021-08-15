#include "pch.h"
#include "catch.hpp"
#include <Uri.hpp>
#include <string>

using namespace piddle;

TEST_CASE( "uri test", "[uri]" ) {

    uri t("http://user@authority/path?q1=1&q2=2#fragment");
    
    REQUIRE( t.scheme() == std::string("http") );
    REQUIRE( t.user() == std::string("user") );
    REQUIRE( t.authority() == std::string("authority") );
    REQUIRE( t.path() == std::string("/path") );

    REQUIRE( t.segment_count() == 1);
    REQUIRE( t.segment(0) == std::string("path") );
    REQUIRE( t.is_relative() == false );
    REQUIRE( t.fragment() == std::string("fragment") );

    REQUIRE( t.query_count() == 2);
    REQUIRE( t.query("q1") == "1");
    REQUIRE( t.query("q2") == "2");
    REQUIRE( t.query("q3") == "");

    REQUIRE( t.query(0) == query_segment("q1","1"));
    REQUIRE( t.query(1) == query_segment("q2","2"));

    t.set("/");
    REQUIRE( t.scheme() == "" );
    REQUIRE( t.user() == "");
    REQUIRE( t.authority() == "");
    REQUIRE( t.path() == std::string("/") );

    REQUIRE( t.segment_count() == 0);
    REQUIRE( t.is_relative() == false );
    REQUIRE( t.fragment() == "");

    REQUIRE( t.query_count() == 0);
    REQUIRE( t.query("q1") == "");

    t.set("");
    REQUIRE( t.scheme() == "" );
    REQUIRE( t.user() == "");
    REQUIRE( t.authority() == "");
    REQUIRE( t.path() == std::string("") );

    REQUIRE( t.segment_count() == 0);
    REQUIRE( t.is_relative() == true );
    REQUIRE( t.fragment() == "");

    REQUIRE( t.query_count() == 0);
    REQUIRE( t.query("q1") == "");

    t.set("a");
    REQUIRE( t.scheme() == "" );
    REQUIRE( t.user() == "");
    REQUIRE( t.authority() == "");
    REQUIRE( t.path() == std::string("a") );

    REQUIRE( t.segment_count() == 1);
    REQUIRE( t.segment(0) == "a");
    REQUIRE( t.is_relative() == true );
    REQUIRE( t.fragment() == "");

    REQUIRE( t.query_count() == 0);
    REQUIRE( t.query("q1") == "");

    t.set("a/b#c");
    REQUIRE( t.is_relative() == true);
    REQUIRE( t.segment_count() == 2);
    REQUIRE( t.segment(0) == "a");
    REQUIRE( t.segment(1) == "b");
    REQUIRE( t.fragment() == "c");

    t.set("/a/b#c");
    REQUIRE( t.is_relative() == false);
    REQUIRE( t.segment_count() == 2);
    REQUIRE( t.segment(0) == "a");
    REQUIRE( t.segment(1) == "b");
    REQUIRE( t.fragment() == "c");


}