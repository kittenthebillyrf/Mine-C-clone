#pragma once
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace mc::assets::json {
struct Value {
    using Array=std::vector<Value>;
    using Object=std::map<std::string,Value>;
    using Data=std::variant<std::nullptr_t,bool,double,std::string,Array,Object>;
    Data data=nullptr;
    bool is_object() const { return std::holds_alternative<Object>(data); }
    bool is_array() const { return std::holds_alternative<Array>(data); }
    bool is_string() const { return std::holds_alternative<std::string>(data); }
    bool is_number() const { return std::holds_alternative<double>(data); }
    bool is_bool() const { return std::holds_alternative<bool>(data); }
    const Value* get(const std::string& k) const { if(!is_object()) return nullptr; const auto& o=std::get<Object>(data); auto i=o.find(k); return i==o.end()?nullptr:&i->second; }
    const Value* at(std::size_t i) const { if(!is_array()) return nullptr; const auto& a=std::get<Array>(data); return i<a.size()?&a[i]:nullptr; }
    const std::string* string() const { return is_string()?&std::get<std::string>(data):nullptr; }
    const double* number() const { return is_number()?&std::get<double>(data):nullptr; }
    const bool* boolean() const { return is_bool()?&std::get<bool>(data):nullptr; }
    const Array* array() const { return is_array()?&std::get<Array>(data):nullptr; }
};
struct ParseResult { Value value; std::size_t error_offset=0; std::string error; explicit operator bool() const { return error.empty(); } };
class Parser {
    const std::string& s; std::size_t p=0; std::string error;
    void ws(){ while(p<s.size()&&(s[p]==' '||s[p]=='\n'||s[p]=='\r'||s[p]=='\t'))++p; }
    bool bad(const char* e){ if(error.empty())error=e; return false; }
    bool take(char c){ if(p>=s.size()||s[p]!=c)return false; ++p; return true; }
    static void utf8(std::string& o,unsigned c){
        if(c<=0x7f)o.push_back(char(c));
        else if(c<=0x7ff){o.push_back(char(0xc0|(c>>6)));o.push_back(char(0x80|(c&63)));}
        else if(c<=0xffff){o.push_back(char(0xe0|(c>>12)));o.push_back(char(0x80|((c>>6)&63)));o.push_back(char(0x80|(c&63)));}
        else{o.push_back(char(0xf0|(c>>18)));o.push_back(char(0x80|((c>>12)&63)));o.push_back(char(0x80|((c>>6)&63)));o.push_back(char(0x80|(c&63)));}
    }
    bool hex4(unsigned& n){ if(p+4>s.size())return false; n=0; for(int i=0;i<4;++i){char c=s[p++];n<<=4;if(c>='0'&&c<='9')n|=unsigned(c-'0');else if(c>='a'&&c<='f')n|=unsigned(c-'a'+10);else if(c>='A'&&c<='F')n|=unsigned(c-'A'+10);else return false;}return true;}
    bool str(std::string& o){
        if(!take('"'))return bad("expected string");
        while(p<s.size()){unsigned char c=static_cast<unsigned char>(s[p++]);if(c=='"')return true;if(c<32)return bad("control character");
            if(c!='\\'){o.push_back(char(c));continue;}if(p==s.size())return bad("incomplete escape");
            char e=s[p++];switch(e){case '"':o.push_back('"');break;case '\\':o.push_back('\\');break;case '/':o.push_back('/');break;case 'b':o.push_back('\b');break;case 'f':o.push_back('\f');break;case 'n':o.push_back('\n');break;case 'r':o.push_back('\r');break;case 't':o.push_back('\t');break;
            case 'u':{unsigned cp=0;if(!hex4(cp))return bad("invalid unicode escape");if(cp>=0xd800&&cp<=0xdbff){unsigned lo=0;if(p+2>s.size()||s[p]!='\\'||s[p+1]!='u')return bad("missing low surrogate");p+=2;if(!hex4(lo)||lo<0xdc00||lo>0xdfff)return bad("invalid low surrogate");cp=0x10000+((cp-0xd800)<<10)+(lo-0xdc00);}else if(cp>=0xdc00&&cp<=0xdfff)return bad("unexpected low surrogate");utf8(o,cp);break;}
            default:return bad("invalid string escape");}}
        return bad("unterminated string");
    }
    bool number(Value& v){
        std::size_t b=p;take('-');
        if(take('0')){if(p<s.size()&&std::isdigit((unsigned char)s[p]))return bad("leading zero");}
        else{if(p==s.size()||s[p]<'1'||s[p]>'9')return bad("invalid number");while(p<s.size()&&std::isdigit((unsigned char)s[p]))++p;}
        if(take('.')){auto q=p;while(p<s.size()&&std::isdigit((unsigned char)s[p]))++p;if(q==p)return bad("missing fractional digits");}
        if(p<s.size()&&(s[p]=='e'||s[p]=='E')){++p;if(p<s.size()&&(s[p]=='+'||s[p]=='-'))++p;auto q=p;while(p<s.size()&&std::isdigit((unsigned char)s[p]))++p;if(q==p)return bad("missing exponent digits");}
        auto t=s.substr(b,p-b);char* end=nullptr;double n=std::strtod(t.c_str(),&end);if(end!=t.c_str()+t.size()||!std::isfinite(n))return bad("number out of range");v.data=n;return true;
    }
    bool value(Value& v,unsigned d){
        if(d>128)return bad("maximum nesting exceeded");ws();if(p==s.size())return bad("unexpected end");char c=s[p];
        if(c=='"'){std::string x;if(!str(x))return false;v.data=std::move(x);return true;}
        if(c=='{'){++p;Value::Object o;ws();if(take('}')){v.data=std::move(o);return true;}for(;;){ws();std::string k;if(!str(k))return false;ws();if(!take(':'))return bad("expected colon");Value x;if(!value(x,d+1))return false;if(!o.emplace(std::move(k),std::move(x)).second)return bad("duplicate key");ws();if(take('}'))break;if(!take(','))return bad("expected comma");}v.data=std::move(o);return true;}
        if(c=='['){++p;Value::Array a;ws();if(take(']')){v.data=std::move(a);return true;}for(;;){Value x;if(!value(x,d+1))return false;a.push_back(std::move(x));ws();if(take(']'))break;if(!take(','))return bad("expected comma");}v.data=std::move(a);return true;}
        if(s.compare(p,4,"true")==0){p+=4;v.data=true;return true;}if(s.compare(p,5,"false")==0){p+=5;v.data=false;return true;}if(s.compare(p,4,"null")==0){p+=4;v.data=nullptr;return true;}
        if(c=='-'||(c>='0'&&c<='9'))return number(v);return bad("unexpected token");
    }
public:
    explicit Parser(const std::string& source):s(source){}
    ParseResult parse(){ParseResult r;ws();if(!value(r.value,0)){r.error_offset=p;r.error=error;return r;}ws();if(p!=s.size()){r.error_offset=p;r.error="trailing characters";}return r;}
};
inline ParseResult parse(const std::string& source){return Parser(source).parse();}
} // namespace mc::assets::json
