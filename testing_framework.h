#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>

using namespace std::string_literals;

template <typename F>
concept TestFunction = requires {
    requires requires(F f) { f(); };
};

template <typename T>
concept Dictionary = requires {
    typename T::key_type;
    typename T::mapped_type;
};

template <Dictionary Dict>
void Print(std::ostream& out, const Dict& dict) {
    bool is_first = true;
    for(const auto& [key, value] : dict) {
        if(is_first) {
            out << key << ": "s << value;
            is_first = false;
        } else {
            out << ", "s << key << ": "s << value; 
        }
    }
}

template <typename Container>
void Print(std::ostream& out, const Container& container) {
    bool is_first = true;
    for(const auto& elem : container) {
        if(is_first) {
            out << elem;
            is_first = false;
        } else {
            out << ", "s << elem; 
        }
    }
}

template <typename Elem>
std::ostream& operator<<(std::ostream& out, const std::vector<Elem>& vector) {
    out << '[';
    Print(out, vector);
    out << ']';

    return out;
}

template <typename Elem>
std::ostream& operator<<(std::ostream& out, const std::set<Elem>& set) {
    out << '{';
    Print(out, set);
    out << '}';
    return out;
}

template <typename Key, typename Value>
std::ostream& operator<<(std::ostream& out, const std::map<Key, Value>& map) {
    out << '{';
    Print(out, map);
    out << '}';
    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file, 
                     const std::string& func, unsigned line, const std::string& hint = "") {
    if (t != u) {
        std::cerr << std::boolalpha;
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s);
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T>
void AssertImpl(const T& t, const std::string& t_str, const std::string& file, const std::string& func,  
                unsigned line, const std::string& hint = "") {
    if(t == false) {
        std::cerr << std::boolalpha;
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT("s << t_str << ") "s << "failed. "s;    
        if(!hint.empty()) {
            std::cerr << "Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

#define ASSERT(a) AssertImpl((a), #a, __FILE__, __FUNCTION__, __LINE__);
#define ASSERT_HINT(a, hint) AssertImpl((a), #a, __FILE__, __FUNCTION__, __LINE__, (hint));

template <TestFunction Function>
void RunTestImpl(Function f, const std::string& f_name) {
    f();
    std::cerr << f_name << " OK"s;
    std::cerr << std::endl;
}

#define RUN_TEST(func)  RunTestImpl(func, #func);
