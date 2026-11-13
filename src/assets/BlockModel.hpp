#pragma once
#include "Json.hpp"
#include <array>
#include <map>
#include <string>
#include <vector>

namespace mc::assets {
struct ModelVec3 { double x=0,y=0,z=0; };
struct ModelFace {
    std::array<double,4> uv{0,0,16,16};
    int rotation=0;
    int tint_index=-1;
    std::string texture;
    std::string cull_face;
};
struct ModelElementRotation {
    ModelVec3 origin{8,8,8};
    char axis='y';
    double angle=0;
    bool rescale=false;
};
struct ModelElement {
    ModelVec3 from{},to{16,16,16};
    bool shade=true;
    bool has_rotation=false;
    ModelElementRotation rotation;
    std::map<std::string,ModelFace> faces;
};
struct BlockModel {
    std::string parent;
    std::map<std::string,std::string> textures;
    std::vector<ModelElement> elements;
    bool has_elements=false;
    bool ambient_occlusion=true;
};
struct ModelParseResult {
    BlockModel model;
    std::string error;
    explicit operator bool() const { return error.empty(); }
};
namespace detail {
inline bool read_number(const json::Value* v,double& out) {
    if(!v||!v->number())return false;out=*v->number();return true;
}
inline bool read_string(const json::Value* v,std::string& out) {
    if(!v||!v->string())return false;out=*v->string();return true;
}
inline bool read_bool(const json::Value* v,bool& out) {
    if(!v||!v->boolean())return false;out=*v->boolean();return true;
}
inline bool read_vec3(const json::Value* v,ModelVec3& out) {
    if(!v||!v->array()||v->array()->size()!=3)return false;
    return read_number(v->at(0),out.x)&&read_number(v->at(1),out.y)&&read_number(v->at(2),out.z);
}
inline bool read_uv(const json::Value* v,std::array<double,4>& out) {
    if(!v||!v->array()||v->array()->size()!=4)return false;
    for(std::size_t i=0;i<4;++i)if(!read_number(v->at(i),out[i]))return false;
    return true;
}
inline bool parse_face(const json::Value& value,ModelFace& face) {
    if(!value.is_object())return false;
    if(const auto* uv=value.get("uv"))if(!read_uv(uv,face.uv))return false;
    if(const auto* rotation=value.get("rotation")) {
        double n=0;if(!read_number(rotation,n)||n<0||n>270||std::fmod(n,90.0)!=0)return false;
        face.rotation=int(n);
    }
    if(const auto* tint=value.get("tintindex")) {
        double n=0;if(!read_number(tint,n)||std::floor(n)!=n)return false;
        face.tint_index=int(n);
    }
    if(const auto* texture=value.get("texture"))if(!read_string(texture,face.texture))return false;
    if(const auto* cull=value.get("cullface"))if(!read_string(cull,face.cull_face))return false;
    return !face.texture.empty();
}
inline bool parse_element(const json::Value& value,ModelElement& element) {
    if(!value.is_object()||!read_vec3(value.get("from"),element.from)||
       !read_vec3(value.get("to"),element.to))return false;
    const auto valid=[](double n){return n>=-16.0&&n<=32.0;};
    if(!valid(element.from.x)||!valid(element.from.y)||!valid(element.from.z)||
       !valid(element.to.x)||!valid(element.to.y)||!valid(element.to.z))return false;
    if(const auto* shade=value.get("shade"))if(!read_bool(shade,element.shade))return false;
    if(const auto* rotation=value.get("rotation")) {
        if(!rotation->is_object()||!read_vec3(rotation->get("origin"),element.rotation.origin))return false;
        std::string axis;if(!read_string(rotation->get("axis"),axis)||axis.size()!=1||
            (axis[0]!='x'&&axis[0]!='y'&&axis[0]!='z'))return false;
        element.rotation.axis=axis[0];
        if(!read_number(rotation->get("angle"),element.rotation.angle))return false;
        if(element.rotation.angle!=-45&&element.rotation.angle!=-22.5&&
           element.rotation.angle!=0&&element.rotation.angle!=22.5&&element.rotation.angle!=45)return false;
        if(const auto* rescale=rotation->get("rescale"))if(!read_bool(rescale,element.rotation.rescale))return false;
        element.has_rotation=true;
    }
    const auto* faces=value.get("faces");
    if(!faces||!faces->object())return false;
    for(const auto& entry:*faces->object()) {
        static const std::vector<std::string> directions={"down","up","north","south","west","east"};
        if(std::find(directions.begin(),directions.end(),entry.first)==directions.end())return false;
        ModelFace face;if(!parse_face(entry.second,face))return false;
        element.faces.emplace(entry.first,std::move(face));
    }
    return !element.faces.empty();
}
} // namespace detail
inline ModelParseResult parse_block_model(const std::string& source) {
    ModelParseResult result;
    const auto parsed=json::parse(source);
    if(!parsed){result.error="JSON at "+std::to_string(parsed.error_offset)+": "+parsed.error;return result;}
    if(!parsed.value.is_object()){result.error="model root must be an object";return result;}
    if(const auto* parent=parsed.value.get("parent"))if(!detail::read_string(parent,result.model.parent)){result.error="parent must be a string";return result;}
    if(const auto* ao=parsed.value.get("ambientocclusion"))if(!detail::read_bool(ao,result.model.ambient_occlusion)){result.error="ambientocclusion must be boolean";return result;}
    if(const auto* textures=parsed.value.get("textures")) {
        if(!textures->object()){result.error="textures must be an object";return result;}
        for(const auto& entry:*textures->object()) {
            std::string texture;if(!detail::read_string(&entry.second,texture)||texture.empty()){result.error="texture values must be non-empty strings";return result;}
            result.model.textures.emplace(entry.first,std::move(texture));
        }
    }
    if(const auto* elements=parsed.value.get("elements")) {
        if(!elements->array()){result.error="elements must be an array";return result;}
        result.model.has_elements=true;
        for(const auto& value:*elements->array()) {
            ModelElement element;if(!detail::parse_element(value,element)){result.error="invalid model element";return result;}
            result.model.elements.push_back(std::move(element));
        }
    }
    return result;
}
} // namespace mc::assets
