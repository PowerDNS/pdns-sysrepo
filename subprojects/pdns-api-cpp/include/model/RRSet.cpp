/**
 * PowerDNS Authoritative HTTP API
 * No description provided (generated by Openapi Generator https://github.com/openapitools/openapi-generator)
 *
 * The version of the OpenAPI document: 0.0.13
 *
 * NOTE: This class is auto generated by OpenAPI-Generator 4.2.1.
 * https://openapi-generator.tech
 * Do not edit the class manually.
 */



#include "RRSet.h"

namespace org {
namespace openapitools {
namespace client {
namespace model {




RRSet::RRSet()
{
    m_Name = utility::conversions::to_string_t("");
    m_Type = utility::conversions::to_string_t("");
    m_Ttl = 0;
    m_Changetype = utility::conversions::to_string_t("");
    m_CommentsIsSet = false;
}

RRSet::~RRSet()
{
}

void RRSet::validate()
{
    // TODO: implement validation
}

web::json::value RRSet::toJson() const
{
    web::json::value val = web::json::value::object();

    val[utility::conversions::to_string_t("name")] = ModelBase::toJson(m_Name);
    val[utility::conversions::to_string_t("type")] = ModelBase::toJson(m_Type);
    val[utility::conversions::to_string_t("ttl")] = ModelBase::toJson(m_Ttl);
    val[utility::conversions::to_string_t("changetype")] = ModelBase::toJson(m_Changetype);
    {
        std::vector<web::json::value> jsonArray;
        for( auto& item : m_Records )
        {
            jsonArray.push_back(ModelBase::toJson(item));
        }
        val[utility::conversions::to_string_t("records")] = web::json::value::array(jsonArray);
    }
    {
        std::vector<web::json::value> jsonArray;
        for( auto& item : m_Comments )
        {
            jsonArray.push_back(ModelBase::toJson(item));
        }
        if(jsonArray.size() > 0)
        {
            val[utility::conversions::to_string_t("comments")] = web::json::value::array(jsonArray);
        }
    }

    return val;
}

void RRSet::fromJson(const web::json::value& val)
{
    setName(ModelBase::stringFromJson(val.at(utility::conversions::to_string_t("name"))));
    setType(ModelBase::stringFromJson(val.at(utility::conversions::to_string_t("type"))));
    setTtl(ModelBase::int32_tFromJson(val.at(utility::conversions::to_string_t("ttl"))));
    setChangetype(ModelBase::stringFromJson(val.at(utility::conversions::to_string_t("changetype"))));
    {
        m_Records.clear();
        std::vector<web::json::value> jsonArray;
        for( auto& item : val.at(utility::conversions::to_string_t("records")).as_array() )
        {
            if(item.is_null())
            {
                m_Records.push_back( std::shared_ptr<Record>(nullptr) );
            }
            else
            {
                std::shared_ptr<Record> newItem(new Record());
                newItem->fromJson(item);
                m_Records.push_back( newItem );
            }
        }
    }
    {
        m_Comments.clear();
        std::vector<web::json::value> jsonArray;
        if(val.has_field(utility::conversions::to_string_t("comments")))
        {
        for( auto& item : val.at(utility::conversions::to_string_t("comments")).as_array() )
        {
            if(item.is_null())
            {
                m_Comments.push_back( std::shared_ptr<Comment>(nullptr) );
            }
            else
            {
                std::shared_ptr<Comment> newItem(new Comment());
                newItem->fromJson(item);
                m_Comments.push_back( newItem );
            }
        }
        }
    }
}

void RRSet::toMultipart(std::shared_ptr<MultipartFormData> multipart, const utility::string_t& prefix) const
{
    utility::string_t namePrefix = prefix;
    if(namePrefix.size() > 0 && namePrefix.substr(namePrefix.size() - 1) != utility::conversions::to_string_t("."))
    {
        namePrefix += utility::conversions::to_string_t(".");
    }

    multipart->add(ModelBase::toHttpContent(namePrefix + utility::conversions::to_string_t("name"), m_Name));
    multipart->add(ModelBase::toHttpContent(namePrefix + utility::conversions::to_string_t("type"), m_Type));
    multipart->add(ModelBase::toHttpContent(namePrefix + utility::conversions::to_string_t("ttl"), m_Ttl));
    multipart->add(ModelBase::toHttpContent(namePrefix + utility::conversions::to_string_t("changetype"), m_Changetype));
    {
        std::vector<web::json::value> jsonArray;
        for( auto& item : m_Records )
        {
            jsonArray.push_back(ModelBase::toJson(item));
        }
        multipart->add(ModelBase::toHttpContent(namePrefix + utility::conversions::to_string_t("records"), web::json::value::array(jsonArray), utility::conversions::to_string_t("application/json")));
            }
    {
        std::vector<web::json::value> jsonArray;
        for( auto& item : m_Comments )
        {
            jsonArray.push_back(ModelBase::toJson(item));
        }
        
        if(jsonArray.size() > 0)
        {
            multipart->add(ModelBase::toHttpContent(namePrefix + utility::conversions::to_string_t("comments"), web::json::value::array(jsonArray), utility::conversions::to_string_t("application/json")));
        }
    }
}

void RRSet::fromMultiPart(std::shared_ptr<MultipartFormData> multipart, const utility::string_t& prefix)
{
    utility::string_t namePrefix = prefix;
    if(namePrefix.size() > 0 && namePrefix.substr(namePrefix.size() - 1) != utility::conversions::to_string_t("."))
    {
        namePrefix += utility::conversions::to_string_t(".");
    }

    setName(ModelBase::stringFromHttpContent(multipart->getContent(utility::conversions::to_string_t("name"))));
    setType(ModelBase::stringFromHttpContent(multipart->getContent(utility::conversions::to_string_t("type"))));
    setTtl(ModelBase::int32_tFromHttpContent(multipart->getContent(utility::conversions::to_string_t("ttl"))));
    setChangetype(ModelBase::stringFromHttpContent(multipart->getContent(utility::conversions::to_string_t("changetype"))));
    {
        m_Records.clear();

        web::json::value jsonArray = web::json::value::parse(ModelBase::stringFromHttpContent(multipart->getContent(utility::conversions::to_string_t("records"))));
        for( auto& item : jsonArray.as_array() )
        {
            if(item.is_null())
            {
                m_Records.push_back( std::shared_ptr<Record>(nullptr) );
            }
            else
            {
                std::shared_ptr<Record> newItem(new Record());
                newItem->fromJson(item);
                m_Records.push_back( newItem );
            }
        }
    }
    {
        m_Comments.clear();
        if(multipart->hasContent(utility::conversions::to_string_t("comments")))
        {

        web::json::value jsonArray = web::json::value::parse(ModelBase::stringFromHttpContent(multipart->getContent(utility::conversions::to_string_t("comments"))));
        for( auto& item : jsonArray.as_array() )
        {
            if(item.is_null())
            {
                m_Comments.push_back( std::shared_ptr<Comment>(nullptr) );
            }
            else
            {
                std::shared_ptr<Comment> newItem(new Comment());
                newItem->fromJson(item);
                m_Comments.push_back( newItem );
            }
        }
        }
    }
}

utility::string_t RRSet::getName() const
{
    return m_Name;
}

void RRSet::setName(const utility::string_t& value)
{
    m_Name = value;
    
}

utility::string_t RRSet::getType() const
{
    return m_Type;
}

void RRSet::setType(const utility::string_t& value)
{
    m_Type = value;
    
}

int32_t RRSet::getTtl() const
{
    return m_Ttl;
}

void RRSet::setTtl(int32_t value)
{
    m_Ttl = value;
    
}

utility::string_t RRSet::getChangetype() const
{
    return m_Changetype;
}

void RRSet::setChangetype(const utility::string_t& value)
{
    m_Changetype = value;
    
}

std::vector<std::shared_ptr<Record>>& RRSet::getRecords()
{
    return m_Records;
}

void RRSet::setRecords(const std::vector<std::shared_ptr<Record>>& value)
{
    m_Records = value;
    
}

std::vector<std::shared_ptr<Comment>>& RRSet::getComments()
{
    return m_Comments;
}

void RRSet::setComments(const std::vector<std::shared_ptr<Comment>>& value)
{
    m_Comments = value;
    m_CommentsIsSet = true;
}

bool RRSet::commentsIsSet() const
{
    return m_CommentsIsSet;
}

void RRSet::unsetComments()
{
    m_CommentsIsSet = false;
}

}
}
}
}

